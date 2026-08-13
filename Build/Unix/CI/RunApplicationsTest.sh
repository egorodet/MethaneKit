#!/bin/bash
# Run all Methane applications one by one from the build directory and verify that:
#   - application does not exit or crash during the test run period (10 seconds by default);
#   - application console and debug output does not contain graphics API validation errors,
#     assertion failures, sanitizer reports or unhandled exception messages.
#
# Usage: RunApplicationsTest.sh [OPTIONS] BUILD_DIR
#   BUILD_DIR is either a CMake build directory (Build/Output/<PRESET>/Build),
#   an install directory or any parent directory containing built Methane applications.
#
# Applications are searched as executables named "Methane*" (excluding unit-test "*Test"
# binaries) under the BUILD_DIR/Apps sub-directory, or under the whole BUILD_DIR when
# no "Apps" sub-directory is present. On MacOS "Methane*.app" bundles are discovered
# and their bundled executables are launched directly to capture their console output.
#
# Note on debug output:
#   Vulkan and Metal validation messages are printed by Methane to the platform debug output,
#   which is the process stdout on Linux and NSLog (stderr) on MacOS, so it is captured here.
#   On Windows (Git Bash / MSYS) debug output goes to OutputDebugString and is NOT captured
#   without a debugger attached: the Khronos validation layer is redirected to stdout with
#   VK_KHRONOS_VALIDATION_* environment variables (see --no-gfx-validation-env), but DirectX
#   debug layer messages remain invisible to this script.

set -o pipefail

SCRIPT_NAME=$(basename "$0")

# ---------------------------------------------------------------------------
# Default settings
# ---------------------------------------------------------------------------

BUILD_DIR=""
OUTPUT_DIR=""
CONFIG_NAME=""
TIMEOUT_SECONDS=10
GRACE_SECONDS=5
MAX_REPORTED_MATCHES=5
REPORTED_CONTEXT_LINES=4
FAIL_ON_WARNINGS=false
ALLOW_EARLY_EXIT=false
LIST_ONLY=false
VERBOSE=false
USE_COLOR=auto
USE_XVFB=auto
SET_GFX_VALIDATION_ENV=true

APP_ARGS=()
INCLUDE_PATTERNS=()
EXCLUDE_PATTERNS=()
EXTRA_ERROR_PATTERNS=()
EXTRA_WARNING_PATTERNS=()
EXTRA_IGNORE_PATTERNS=()

# Error patterns matched (as extended regular expressions) in the application output.
# NOTE: patterns are matched by awk, so POSIX ERE syntax is used without GNU extensions.
ERROR_PATTERNS=(
    '^Error [A-Za-z]'                 # Methane Vulkan debug-utils messenger block of Error severity
    'Validation Error'                # Khronos validation layer direct output
    'VALIDATION.*ERROR'
    'D3D12 ERROR'                     # DirectX 12 debug layer
    'D3D12 CORRUPTION'
    'DXGI ERROR'
    'DXGI CORRUPTION'
    'Metal API Validation'            # Metal validation layer
    'validation fail'
    'Validation Fail'
    '[Aa]ssertion fail'
    'Assert.*failed'
    'terminate called'
    'Segmentation fault'
    'Bus error'
    'Stack dump'
    'libc[+][+]abi'
    'AddressSanitizer'
    'LeakSanitizer'
    'ThreadSanitizer'
    'MemorySanitizer'
    'UndefinedBehaviorSanitizer'
    'runtime error:'
    'Unhandled exception'
    'EXCEPTION_ACCESS_VIOLATION'
    '[Ff]atal error'
    'Failed to parse command line'
    'error while loading shared libraries'   # dynamic library is missing next to the executable
    'cannot open shared object file'
    'Library not loaded'
    'Symbol not found'
    'image not found'
)

# Warning patterns are only reported, unless --fail-on-warnings is used.
WARNING_PATTERNS=(
    '^Warning [A-Za-z]'               # Methane Vulkan debug-utils messenger block of Warning severity
    'D3D12 WARNING'
    'DXGI WARNING'
    'Performance Warning'
    'WARNING:.*[Vv]alidation'
)

# Known harmless messages, which are excluded from error and warning matches.
IGNORE_PATTERNS=(
    # Informational message printed by Metal on startup when MTL_DEBUG_LAYER=1 is set
    # (see setup_validation_environment), it is not a validation failure.
    'Metal API Validation (Enabled|Disabled)'
    'loader_get_json'
    'terminator_CreateInstance'
    'Removing layer'
    'Failed to open JSON file'
    'lavapipe is not a conformant'
    'MESA-INTEL'
    'libGL error'
)

# ---------------------------------------------------------------------------
# Output helpers
# ---------------------------------------------------------------------------

COLOR_RESET=''; COLOR_RED=''; COLOR_GREEN=''; COLOR_YELLOW=''; COLOR_BLUE=''; COLOR_BOLD=''

init_colors() {
    if [ "$USE_COLOR" == "never" ] || { [ "$USE_COLOR" == "auto" ] && [ ! -t 1 ]; }; then
        return
    fi
    COLOR_RESET=$'\033[0m'
    COLOR_RED=$'\033[31m'
    COLOR_GREEN=$'\033[32m'
    COLOR_YELLOW=$'\033[33m'
    COLOR_BLUE=$'\033[34m'
    COLOR_BOLD=$'\033[1m'
}

print_header() {
    echo "============================================================================="
    echo "$1"
    echo "============================================================================="
}

print_info()    { echo "$1"; }
print_verbose() { if [ "$VERBOSE" == true ]; then echo "  $1"; fi }
print_pass()    { echo "${COLOR_GREEN}$1${COLOR_RESET}"; }
print_warn()    { echo "${COLOR_YELLOW}$1${COLOR_RESET}"; }
print_fail()    { echo "${COLOR_RED}$1${COLOR_RESET}"; }

print_usage() {
    cat <<EOF
Usage: $SCRIPT_NAME [OPTIONS] BUILD_DIR

Runs all Methane applications found in BUILD_DIR one by one, keeps each of them running
for a while and fails if an application exits early, crashes or prints validation errors.

Options:
  -t, --timeout SECONDS     Run each application for that many seconds (default: $TIMEOUT_SECONDS)
  -g, --grace SECONDS       Wait for graceful termination before killing (default: $GRACE_SECONDS)
  -c, --config CONFIG       Build configuration sub-directory to select binaries from
                            in multi-config generator builds (Debug, Release, ...)
  -o, --output-dir DIR      Directory to write application logs to
                            (default: BUILD_DIR/AppsTestResults)
  -i, --include REGEX       Run only applications with names matching REGEX (repeatable)
  -x, --exclude REGEX       Skip applications with names matching REGEX (repeatable)
  -A, --app-args "ARGS"     Extra command line arguments passed to every application,
                            e.g. --app-args "-w 640 480". Arguments after -- are added too.
  -E, --error-pattern RE    Additional error pattern to search in the output (repeatable)
  -W, --warning-pattern RE  Additional warning pattern to search in the output (repeatable)
  -I, --ignore-pattern RE   Additional pattern of messages to ignore (repeatable)
      --fail-on-warnings    Treat validation warnings as errors
      --allow-early-exit    Do not fail when application exits before the timeout with code 0
      --xvfb / --no-xvfb    Force or disable running under Xvfb virtual display on Linux
                            (auto by default: used when DISPLAY is not set and Xvfb exists)
      --no-gfx-validation-env  Do not set VK_KHRONOS_VALIDATION_* / MTL_DEBUG_LAYER variables
      --context LINES       Number of output lines printed after each match (default: $REPORTED_CONTEXT_LINES)
      --max-matches COUNT   Maximum number of matches printed per application (default: $MAX_REPORTED_MATCHES)
  -l, --list                List discovered applications and exit
  -v, --verbose             Print more details during the test run
      --color WHEN          Colored output: auto (default), always or never
  -h, --help                Print this help and exit

Exit codes: 0 - all applications passed, 1 - some applications failed,
            2 - command line or environment error, 3 - no applications found.

Examples:
  $SCRIPT_NAME Build/Output/Ninja-Lin-VK-Default/Build
  $SCRIPT_NAME --timeout 30 --fail-on-warnings Build/Output/Xcode-Mac-MTL-Default/Build
  $SCRIPT_NAME -c Release -x ConsoleCompute Build/Output/VS2022-Win64-DX-Default/Build
EOF
}

# ---------------------------------------------------------------------------
# Command line parsing
# ---------------------------------------------------------------------------

parse_command_line() {
    while [ $# -ne 0 ]; do
        case "$1" in
            -t|--timeout)          TIMEOUT_SECONDS="$2"; shift ;;
            -g|--grace)            GRACE_SECONDS="$2"; shift ;;
            -c|--config)           CONFIG_NAME="$2"; shift ;;
            -o|--output-dir)       OUTPUT_DIR="$2"; shift ;;
            -i|--include)          INCLUDE_PATTERNS[${#INCLUDE_PATTERNS[@]}]="$2"; shift ;;
            -x|--exclude)          EXCLUDE_PATTERNS[${#EXCLUDE_PATTERNS[@]}]="$2"; shift ;;
            -E|--error-pattern)    EXTRA_ERROR_PATTERNS[${#EXTRA_ERROR_PATTERNS[@]}]="$2"; shift ;;
            -W|--warning-pattern)  EXTRA_WARNING_PATTERNS[${#EXTRA_WARNING_PATTERNS[@]}]="$2"; shift ;;
            -I|--ignore-pattern)   EXTRA_IGNORE_PATTERNS[${#EXTRA_IGNORE_PATTERNS[@]}]="$2"; shift ;;
            -A|--app-args)
                # Split the argument string by white-spaces into separate application arguments
                for app_arg in $2; do
                    APP_ARGS[${#APP_ARGS[@]}]="$app_arg"
                done
                shift
                ;;
            --fail-on-warnings)    FAIL_ON_WARNINGS=true ;;
            --allow-early-exit)    ALLOW_EARLY_EXIT=true ;;
            --xvfb)                USE_XVFB=always ;;
            --no-xvfb)             USE_XVFB=never ;;
            --no-gfx-validation-env) SET_GFX_VALIDATION_ENV=false ;;
            --context)             REPORTED_CONTEXT_LINES="$2"; shift ;;
            --max-matches)         MAX_REPORTED_MATCHES="$2"; shift ;;
            -l|--list)             LIST_ONLY=true ;;
            -v|--verbose)          VERBOSE=true ;;
            --color)               USE_COLOR="$2"; shift ;;
            -h|--help)             print_usage; exit 0 ;;
            --)
                shift
                while [ $# -ne 0 ]; do
                    APP_ARGS[${#APP_ARGS[@]}]="$1"
                    shift
                done
                break
                ;;
            -*)
                echo "Unknown option: $1" >&2
                echo "Run '$SCRIPT_NAME --help' for usage." >&2
                exit 2
                ;;
            *)
                if [ -n "$BUILD_DIR" ]; then
                    echo "Unexpected argument: $1 (build directory is already set to '$BUILD_DIR')" >&2
                    exit 2
                fi
                BUILD_DIR="$1"
                ;;
        esac
        shift
    done

    if [ -z "$BUILD_DIR" ]; then
        echo "Build directory argument is required!" >&2
        print_usage >&2
        exit 2
    fi
    if [ ! -d "$BUILD_DIR" ]; then
        echo "Build directory does not exist: $BUILD_DIR" >&2
        exit 2
    fi
    if ! echo "$TIMEOUT_SECONDS" | grep -qE '^[0-9]+$' || [ "$TIMEOUT_SECONDS" -lt 1 ]; then
        echo "Invalid timeout value: $TIMEOUT_SECONDS (positive integer number of seconds is expected)" >&2
        exit 2
    fi
    if ! echo "$GRACE_SECONDS" | grep -qE '^[0-9]+$'; then
        echo "Invalid grace period value: $GRACE_SECONDS (integer number of seconds is expected)" >&2
        exit 2
    fi
    if ! echo "$MAX_REPORTED_MATCHES" | grep -qE '^[0-9]+$'; then
        echo "Invalid maximum matches value: $MAX_REPORTED_MATCHES (integer number is expected)" >&2
        exit 2
    fi
    if ! echo "$REPORTED_CONTEXT_LINES" | grep -qE '^[0-9]+$'; then
        echo "Invalid context lines value: $REPORTED_CONTEXT_LINES (integer number is expected)" >&2
        exit 2
    fi

    BUILD_DIR=$(cd "$BUILD_DIR" && pwd)
    if [ -z "$OUTPUT_DIR" ]; then
        OUTPUT_DIR="$BUILD_DIR/AppsTestResults"
    fi
}

# ---------------------------------------------------------------------------
# Platform detection and environment setup
# ---------------------------------------------------------------------------

PLATFORM_NAME=""
SLEEP_INTERVAL=0.25
XVFB_PID=""

detect_platform() {
    case "$(uname -s)" in
        Linux*)                   PLATFORM_NAME=Linux ;;
        Darwin*)                  PLATFORM_NAME=MacOS ;;
        CYGWIN*|MINGW*|MSYS*)     PLATFORM_NAME=Windows ;;
        *)
            echo "Unsupported operating system: $(uname -s)" >&2
            exit 2
            ;;
    esac

    # Fall back to 1 second polling interval if fractional sleep is not supported
    if ! sleep 0.05 >/dev/null 2>&1; then
        SLEEP_INTERVAL=1
    fi
}

setup_validation_environment() {
    if [ "$SET_GFX_VALIDATION_ENV" != true ]; then
        return
    fi
    # Redirect Khronos validation layer messages to the process stdout, so that they are
    # captured in the application log even when Methane debug output is not visible (Windows).
    export VK_KHRONOS_VALIDATION_DEBUG_ACTION=VK_DBG_LAYER_ACTION_LOG_MSG
    export VK_KHRONOS_VALIDATION_LOG_FILENAME=stdout
    export VK_KHRONOS_VALIDATION_REPORT_FLAGS=error,warn,perf
    if [ "$PLATFORM_NAME" == "MacOS" ]; then
        export MTL_DEBUG_LAYER=1
    fi
}

setup_display() {
    if [ "$PLATFORM_NAME" != "Linux" ]; then
        return
    fi
    if [ "$USE_XVFB" == "never" ]; then
        return
    fi
    if [ "$USE_XVFB" == "auto" ] && { [ -n "$DISPLAY" ] || [ -n "$WAYLAND_DISPLAY" ]; }; then
        return
    fi
    if ! command -v Xvfb >/dev/null 2>&1; then
        if [ "$USE_XVFB" == "always" ]; then
            echo "Xvfb is requested with --xvfb, but it is not installed." >&2
            exit 2
        fi
        print_warn "WARNING: DISPLAY is not set and Xvfb is not installed - graphics applications may fail to open a window."
        return
    fi

    local display_number=99
    while [ -e "/tmp/.X${display_number}-lock" ] && [ $display_number -lt 120 ]; do
        display_number=$((display_number + 1))
    done

    print_info "Starting Xvfb virtual display :${display_number} ..."
    Xvfb ":${display_number}" -screen 0 1920x1080x24 >"$OUTPUT_DIR/Xvfb.log" 2>&1 &
    XVFB_PID=$!
    sleep 1
    if ! kill -0 "$XVFB_PID" 2>/dev/null; then
        echo "Failed to start Xvfb virtual display, see log: $OUTPUT_DIR/Xvfb.log" >&2
        XVFB_PID=""
        exit 2
    fi
    export DISPLAY=":${display_number}"
}

# ---------------------------------------------------------------------------
# Applications discovery
# ---------------------------------------------------------------------------

APP_PATHS=()
APP_NAMES=()

matches_any_pattern() { # $1 - text, $2... - patterns
    local text="$1"
    shift
    local pattern
    for pattern in "$@"; do
        if echo "$text" | grep -qE "$pattern"; then
            return 0
        fi
    done
    return 1
}

get_app_name() { # $1 - executable or bundle path
    local app_name
    app_name=$(basename "$1")
    app_name="${app_name%.exe}"
    app_name="${app_name%.app}"
    echo "$app_name"
}

is_application_binary() { # $1 - file path
    local file_path="$1"
    local file_name
    file_name=$(basename "$file_path")

    case "$file_name" in
        Methane*) ;;
        *) return 1 ;;
    esac
    # Unit-test executables are not applications
    case "$file_name" in
        *Test|*Test.exe|*Tests|*Tests.exe) return 1 ;;
    esac

    if [ "$PLATFORM_NAME" == "Windows" ]; then
        case "$file_name" in
            *.exe) ;;
            *) return 1 ;;
        esac
    else
        # Skip libraries, symbol files and other artifacts having a file extension
        case "$file_name" in
            *.*) return 1 ;;
        esac
        [ -x "$file_path" ] || return 1
    fi
    return 0
}

find_application_files() { # $1 - search directory
    if [ "$PLATFORM_NAME" == "MacOS" ]; then
        find "$1" -type d -name 'Methane*.app' -prune 2>/dev/null | LC_ALL=C sort
    else
        find "$1" -type f -name 'Methane*' 2>/dev/null | LC_ALL=C sort
    fi
}

resolve_app_executable() { # $1 - discovered application path
    if [ "$PLATFORM_NAME" == "MacOS" ]; then
        local bundle_name
        bundle_name=$(get_app_name "$1")
        echo "$1/Contents/MacOS/$bundle_name"
    else
        echo "$1"
    fi
}

select_app_by_config() { # $1 - app name, $2... - candidate paths of the same application
    local app_name="$1"
    shift
    local candidates=("$@")
    local selected=""
    local candidate

    if [ -n "$CONFIG_NAME" ]; then
        for candidate in "${candidates[@]}"; do
            case "$candidate" in
                */$CONFIG_NAME/*) selected="$candidate"; break ;;
            esac
        done
        if [ -z "$selected" ]; then
            print_verbose "No '$CONFIG_NAME' configuration binary found for $app_name, using the most recently built one."
        fi
    fi

    if [ -z "$selected" ]; then
        # Multiple configurations are built in the same directory: take the most recent one
        selected=$(ls -td "${candidates[@]}" 2>/dev/null | head -n 1)
        if [ ${#candidates[@]} -gt 1 ]; then
            print_verbose "Selected the most recently built binary of $app_name: $selected"
        fi
    fi
    echo "$selected"
}

discover_applications() {
    local search_dir="$BUILD_DIR"
    if [ -d "$BUILD_DIR/Apps" ]; then
        search_dir="$BUILD_DIR/Apps"
    fi
    print_verbose "Searching applications in $search_dir"

    local found_files=()
    local file_path
    while IFS= read -r file_path; do
        [ -n "$file_path" ] || continue
        if [ "$PLATFORM_NAME" != "MacOS" ] && ! is_application_binary "$file_path"; then
            continue
        fi
        if [ "$PLATFORM_NAME" == "MacOS" ]; then
            case "$(basename "$file_path")" in
                *Test.app|*Tests.app) continue ;;
            esac
        fi
        found_files[${#found_files[@]}]="$file_path"
    done <<EOF
$(find_application_files "$search_dir")
EOF

    if [ ${#found_files[@]} -eq 0 ]; then
        return
    fi

    # Group binaries of the same application built in different configurations and select one of them
    local unique_names
    unique_names=$(for file_path in "${found_files[@]}"; do get_app_name "$file_path"; done | LC_ALL=C sort -u)

    local app_name
    while IFS= read -r app_name; do
        [ -n "$app_name" ] || continue
        if [ ${#INCLUDE_PATTERNS[@]} -gt 0 ] && ! matches_any_pattern "$app_name" "${INCLUDE_PATTERNS[@]}"; then
            print_verbose "Skipping $app_name: does not match include patterns."
            continue
        fi
        if [ ${#EXCLUDE_PATTERNS[@]} -gt 0 ] && matches_any_pattern "$app_name" "${EXCLUDE_PATTERNS[@]}"; then
            print_verbose "Skipping $app_name: matches exclude patterns."
            continue
        fi

        local candidates=()
        for file_path in "${found_files[@]}"; do
            if [ "$(get_app_name "$file_path")" == "$app_name" ]; then
                candidates[${#candidates[@]}]="$file_path"
            fi
        done

        local selected_path
        selected_path=$(select_app_by_config "$app_name" "${candidates[@]}")
        local app_exe_path
        app_exe_path=$(resolve_app_executable "$selected_path")
        if [ ! -x "$app_exe_path" ] && [ ! -f "$app_exe_path" ]; then
            print_warn "WARNING: Skipping $app_name: executable is not found in $selected_path"
            continue
        fi

        APP_NAMES[${#APP_NAMES[@]}]="$app_name"
        APP_PATHS[${#APP_PATHS[@]}]="$app_exe_path"
    done <<EOF
$unique_names
EOF
}

# ---------------------------------------------------------------------------
# Output analysis
# ---------------------------------------------------------------------------

ERROR_REGEX=""
WARNING_REGEX=""
IGNORE_REGEX=""

join_patterns() { # $@ - patterns
    local result=""
    local pattern
    for pattern in "$@"; do
        if [ -z "$result" ]; then
            result="($pattern)"
        else
            result="$result|($pattern)"
        fi
    done
    echo "$result"
}

build_match_regexps() {
    ERROR_REGEX=$(join_patterns "${ERROR_PATTERNS[@]}" ${EXTRA_ERROR_PATTERNS[@]+"${EXTRA_ERROR_PATTERNS[@]}"})
    WARNING_REGEX=$(join_patterns "${WARNING_PATTERNS[@]}" ${EXTRA_WARNING_PATTERNS[@]+"${EXTRA_WARNING_PATTERNS[@]}"})
    IGNORE_REGEX=$(join_patterns "${IGNORE_PATTERNS[@]}" ${EXTRA_IGNORE_PATTERNS[@]+"${EXTRA_IGNORE_PATTERNS[@]}"})
}

count_log_matches() { # $1 - log file, $2 - match regex
    LC_ALL=C awk -v re="$2" -v ign="$IGNORE_REGEX" '
        $0 ~ re && (ign == "" || $0 !~ ign) { matches_count++ }
        END { print matches_count + 0 }
    ' "$1" 2>/dev/null
}

print_log_matches() { # $1 - log file, $2 - match regex, $3 - message prefix
    LC_ALL=C awk -v re="$2" -v ign="$IGNORE_REGEX" -v prefix="$3" \
                 -v max_matches="$MAX_REPORTED_MATCHES" -v context="$REPORTED_CONTEXT_LINES" '
        { lines[NR] = $0 }
        $0 ~ re && (ign == "" || $0 !~ ign) {
            total++
            if (total <= max_matches)
                hits[total] = NR
        }
        END {
            printed_line = 0
            for (i = 1; i <= total && i <= max_matches; i++) {
                first_line = hits[i]
                if (first_line <= printed_line)
                    first_line = printed_line + 1  # do not repeat lines printed as a previous match context
                else if (i > 1)
                    print "        ..."
                for (j = first_line; j <= hits[i] + context && j <= NR; j++)
                    printf("      %s %6d | %s\n", prefix, j, lines[j])
                if (hits[i] + context > printed_line)
                    printed_line = hits[i] + context
            }
            if (total > max_matches)
                printf("      %s ... and %d more match(es), see the full log.\n", prefix, total - max_matches)
        }
    ' "$1" 2>/dev/null
}

# ---------------------------------------------------------------------------
# Application run
# ---------------------------------------------------------------------------

CURRENT_APP_PID=""

wait_for_process_exit() { # $1 - pid, $2 - timeout in seconds; returns 0 if exited, 1 if still running
    local pid="$1"
    local timeout_seconds="$2"
    local polls_count
    local poll_index=0

    polls_count=$(awk -v t="$timeout_seconds" -v i="$SLEEP_INTERVAL" 'BEGIN { printf("%d", t / i) }')
    while [ $poll_index -lt "$polls_count" ]; do
        if ! kill -0 "$pid" 2>/dev/null; then
            return 0
        fi
        sleep "$SLEEP_INTERVAL"
        poll_index=$((poll_index + 1))
    done
    if kill -0 "$pid" 2>/dev/null; then
        return 1
    fi
    return 0
}

terminate_process() { # $1 - pid; returns 0 when terminated gracefully, 1 when killed forcibly
    local pid="$1"
    kill -TERM "$pid" 2>/dev/null
    if wait_for_process_exit "$pid" "$GRACE_SECONDS"; then
        return 0
    fi
    print_verbose "Application did not exit in $GRACE_SECONDS seconds after SIGTERM, sending SIGKILL."
    kill -KILL "$pid" 2>/dev/null
    wait_for_process_exit "$pid" 5
    return 1
}

get_signal_description() { # $1 - exit status
    local signal_number=$(($1 - 128))
    case $signal_number in
        2)  echo "SIGINT" ;;
        4)  echo "SIGILL" ;;
        6)  echo "SIGABRT" ;;
        7)  echo "SIGBUS" ;;
        8)  echo "SIGFPE" ;;
        9)  echo "SIGKILL" ;;
        10) echo "SIGBUS/SIGUSR1" ;;
        11) echo "SIGSEGV" ;;
        13) echo "SIGPIPE" ;;
        15) echo "SIGTERM" ;;
        *)  echo "signal $signal_number" ;;
    esac
}

RESULT_NAMES=()
RESULT_STATES=()
RESULT_DETAILS=()

add_result() { # $1 - app name, $2 - state, $3 - details
    RESULT_NAMES[${#RESULT_NAMES[@]}]="$1"
    RESULT_STATES[${#RESULT_STATES[@]}]="$2"
    RESULT_DETAILS[${#RESULT_DETAILS[@]}]="$3"
}

run_application() { # $1 - app name, $2 - executable path
    local app_name="$1"
    local app_exe_path="$2"
    local log_file="$OUTPUT_DIR/$app_name.log"
    local failure_reasons=""
    local warning_details=""

    print_info "${COLOR_BOLD}--- $app_name${COLOR_RESET}"
    print_verbose "Executable: $app_exe_path"
    print_verbose "Log file:   $log_file"

    : >"$log_file"
    "$app_exe_path" ${APP_ARGS[@]+"${APP_ARGS[@]}"} >"$log_file" 2>&1 </dev/null &
    CURRENT_APP_PID=$!

    local exit_status=0
    local was_terminated=false
    if wait_for_process_exit "$CURRENT_APP_PID" "$TIMEOUT_SECONDS"; then
        wait "$CURRENT_APP_PID"
        exit_status=$?
    else
        was_terminated=true
        terminate_process "$CURRENT_APP_PID"
        wait "$CURRENT_APP_PID" 2>/dev/null
        exit_status=$?
    fi
    CURRENT_APP_PID=""

    # Check the application run time behavior
    if [ "$was_terminated" == true ]; then
        print_verbose "Application was running for $TIMEOUT_SECONDS seconds and was terminated as expected (exit status $exit_status)."
    elif [ $exit_status -gt 128 ]; then
        failure_reasons="crashed with $(get_signal_description $exit_status)"
    elif [ $exit_status -ne 0 ]; then
        failure_reasons="exited with error code $exit_status before the $TIMEOUT_SECONDS seconds timeout"
    elif [ "$ALLOW_EARLY_EXIT" != true ]; then
        failure_reasons="exited on its own before the $TIMEOUT_SECONDS seconds timeout (exit code 0)"
    else
        print_verbose "Application exited with code 0 before the timeout, which is allowed."
    fi

    # Check the application console and debug output
    local errors_count
    local warnings_count
    errors_count=$(count_log_matches "$log_file" "$ERROR_REGEX")
    warnings_count=$(count_log_matches "$log_file" "$WARNING_REGEX")
    [ -n "$errors_count" ]   || errors_count=0
    [ -n "$warnings_count" ] || warnings_count=0

    if [ "$errors_count" -gt 0 ]; then
        if [ -n "$failure_reasons" ]; then
            failure_reasons="$failure_reasons; "
        fi
        failure_reasons="${failure_reasons}${errors_count} error message(s) in console and debug output"
    fi
    if [ "$warnings_count" -gt 0 ]; then
        warning_details="${warnings_count} warning message(s) in console and debug output"
        if [ "$FAIL_ON_WARNINGS" == true ]; then
            if [ -n "$failure_reasons" ]; then
                failure_reasons="$failure_reasons; "
            fi
            failure_reasons="${failure_reasons}${warning_details}"
        fi
    fi

    if [ -n "$failure_reasons" ]; then
        print_fail "  FAILED: $app_name - $failure_reasons"
        if [ "$errors_count" -gt 0 ]; then
            print_fail "    Errors found in $log_file:"
            print_log_matches "$log_file" "$ERROR_REGEX" "E"
        fi
        if [ "$warnings_count" -gt 0 ]; then
            print_warn "    Warnings found in $log_file:"
            print_log_matches "$log_file" "$WARNING_REGEX" "W"
        fi
        if [ "$errors_count" -eq 0 ] && [ "$warnings_count" -eq 0 ]; then
            print_fail "    Last output lines from $log_file:"
            tail -n 10 "$log_file" 2>/dev/null | sed 's/^/      | /'
        fi
        add_result "$app_name" "FAILED" "$failure_reasons"
        return 1
    fi

    if [ "$warnings_count" -gt 0 ]; then
        print_warn "  PASSED with warnings: $app_name - $warning_details"
        print_log_matches "$log_file" "$WARNING_REGEX" "W"
        add_result "$app_name" "WARNING" "$warning_details"
        return 0
    fi

    print_pass "  PASSED: $app_name - was running for $TIMEOUT_SECONDS seconds without errors."
    add_result "$app_name" "PASSED" ""
    return 0
}

# ---------------------------------------------------------------------------
# Cleanup
# ---------------------------------------------------------------------------

cleanup() {
    if [ -n "$CURRENT_APP_PID" ] && kill -0 "$CURRENT_APP_PID" 2>/dev/null; then
        kill -KILL "$CURRENT_APP_PID" 2>/dev/null
    fi
    if [ -n "$XVFB_PID" ] && kill -0 "$XVFB_PID" 2>/dev/null; then
        kill -TERM "$XVFB_PID" 2>/dev/null
    fi
}

interrupt() {
    echo ""
    print_warn "Applications test run was interrupted."
    cleanup
    exit 2
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

parse_command_line "$@"
init_colors
detect_platform
build_match_regexps

trap interrupt INT TERM
trap cleanup EXIT

if ! mkdir -p "$OUTPUT_DIR"; then
    echo "Failed to create output directory: $OUTPUT_DIR" >&2
    exit 2
fi
OUTPUT_DIR=$(cd "$OUTPUT_DIR" && pwd)

discover_applications

if [ ${#APP_PATHS[@]} -eq 0 ]; then
    echo "No Methane applications were found in build directory: $BUILD_DIR" >&2
    exit 3
fi

if [ "$LIST_ONLY" == true ]; then
    print_header "Found ${#APP_PATHS[@]} Methane applications in $BUILD_DIR"
    app_index=0
    while [ $app_index -lt ${#APP_PATHS[@]} ]; do
        echo " - ${APP_NAMES[$app_index]}: ${APP_PATHS[$app_index]}"
        app_index=$((app_index + 1))
    done
    exit 0
fi

setup_validation_environment
setup_display

print_header "Testing ${#APP_PATHS[@]} Methane applications for $PLATFORM_NAME"
echo " * Build directory:  $BUILD_DIR"
echo " * Logs directory:   $OUTPUT_DIR"
echo " * Run time per app: $TIMEOUT_SECONDS seconds"
if [ ${#APP_ARGS[@]} -gt 0 ]; then
    echo " * Application args: ${APP_ARGS[*]}"
fi
if [ -n "$DISPLAY" ]; then
    echo " * Display:          $DISPLAY"
fi
if [ "$PLATFORM_NAME" == "Windows" ]; then
    print_warn " * WARNING: platform debug output is not captured on Windows without a debugger attached,"
    print_warn "            so DirectX 12 debug layer messages can not be verified by this script."
fi
echo "============================================================================="

FAILED_COUNT=0
WARNED_COUNT=0
app_index=0
while [ $app_index -lt ${#APP_PATHS[@]} ]; do
    if ! run_application "${APP_NAMES[$app_index]}" "${APP_PATHS[$app_index]}"; then
        FAILED_COUNT=$((FAILED_COUNT + 1))
    elif [ "${RESULT_STATES[$((${#RESULT_STATES[@]} - 1))]}" == "WARNING" ]; then
        WARNED_COUNT=$((WARNED_COUNT + 1))
    fi
    app_index=$((app_index + 1))
done

PASSED_COUNT=$((${#APP_PATHS[@]} - FAILED_COUNT))

print_header "Applications test summary"
app_index=0
while [ $app_index -lt ${#RESULT_NAMES[@]} ]; do
    result_line=$(printf " %-40s %s" "${RESULT_NAMES[$app_index]}" "${RESULT_STATES[$app_index]}")
    if [ -n "${RESULT_DETAILS[$app_index]}" ]; then
        result_line="$result_line - ${RESULT_DETAILS[$app_index]}"
    fi
    case "${RESULT_STATES[$app_index]}" in
        PASSED)  print_pass "$result_line" ;;
        WARNING) print_warn "$result_line" ;;
        *)       print_fail "$result_line" ;;
    esac
    app_index=$((app_index + 1))
done
echo "-----------------------------------------------------------------------------"
echo " Total: ${#APP_PATHS[@]}, passed: $PASSED_COUNT (with warnings: $WARNED_COUNT), failed: $FAILED_COUNT"
echo " Application logs are written to: $OUTPUT_DIR"
echo "============================================================================="

if [ $FAILED_COUNT -ne 0 ]; then
    exit 1
fi
exit 0
