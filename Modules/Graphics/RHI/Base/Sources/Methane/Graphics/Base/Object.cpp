/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License"),
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/Base/Object.cpp
Base implementation of the named object interface.

******************************************************************************/

#include <Methane/Graphics/Base/Object.h>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <stdexcept>
#include <cassert>

namespace Methane::Graphics::Base
{

void ObjectRegistry::AddGraphicsObject(Rhi::IObject& object)
{
    META_FUNCTION_TASK();
    META_CHECK_NOT_EMPTY_DESCR(object.GetName(), "Can not add graphics object without name to the objects registry.");

    const auto& obj = dynamic_cast<Object&>(object);
    const auto [name_and_object_it, object_added] = m_object_by_name.try_emplace(obj.GetNameRef(), object.GetPtr());
    if (!object_added &&
        !name_and_object_it->second.expired() &&
         name_and_object_it->second.lock().get() != std::addressof(object))
        throw NameConflictException(object.GetName());

    name_and_object_it->second = object.GetPtr();
    object.Connect(*this);
}

void ObjectRegistry::RemoveGraphicsObject(Rhi::IObject& object)
{
    META_FUNCTION_TASK();

    const auto& obj = dynamic_cast<Object&>(object);
    const std::string& object_name = obj.GetNameRef();
    META_CHECK_NOT_EMPTY_DESCR(object_name, "Can not remove graphics object without name to the objects registry.");

    if (m_object_by_name.erase(object_name))
    {
        object.Disconnect(*this);
    }
}

Ptr<Rhi::IObject> ObjectRegistry::GetGraphicsObject(const std::string& object_name) const noexcept
{
    META_FUNCTION_TASK();
    const auto object_by_name_it = m_object_by_name.find(object_name);
    return object_by_name_it == m_object_by_name.end() ? nullptr : object_by_name_it->second.lock();
}

bool ObjectRegistry::HasGraphicsObject(const std::string& object_name) const noexcept
{
    META_FUNCTION_TASK();
    const auto object_by_name_it = m_object_by_name.find(object_name);
    return object_by_name_it != m_object_by_name.end() && !object_by_name_it->second.expired();
}

void ObjectRegistry::OnObjectNameChanged(Rhi::IObject& object, const std::string& old_name)
{
    META_FUNCTION_TASK();
    const auto object_by_name_it = m_object_by_name.find(old_name);
    META_CHECK_TRUE_DESCR(object_by_name_it != m_object_by_name.end(),
                          "renamed object was not found in the objects registry by its old name '{}'", old_name);
    META_CHECK_TRUE_DESCR(object_by_name_it->second.expired(),
                          "object pointer stored in registry by old name '{}' has expired", old_name);
    META_CHECK_TRUE_DESCR(std::addressof(*object_by_name_it->second.lock()) == std::addressof(object),
                          "object stored in the registry by old name '{}' differs from the renamed object", old_name);

    const std::string_view new_name = object.GetName();
    if (new_name.empty())
    {
        m_object_by_name.erase(object_by_name_it);
        object.Disconnect(*this);
        return;
    }

    auto object_node = m_object_by_name.extract(object_by_name_it);
    object_node.key() = new_name;
    m_object_by_name.insert(std::move(object_node));
}

void ObjectRegistry::OnObjectDestroyed(Rhi::IObject& object)
{
    META_FUNCTION_TASK();
    RemoveGraphicsObject(object);
}

Object::Object(std::string_view name)
    : m_name(name)
{ }

Object::~Object()
{
    META_FUNCTION_TASK();
    try
    {
        Emit(&Rhi::IObjectCallback::OnObjectDestroyed, *this);
    }
    catch (const std::exception& e)
    {
        META_UNUSED(e);
        META_LOG("WARNING: Unexpected error during object destruction: {}", e.what());
        assert(false);
    }
}

Ptr<Rhi::IObject> Object::GetPtr()
{
    META_FUNCTION_TASK();
    return std::dynamic_pointer_cast<IObject>(shared_from_this());
}

bool Object::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (m_name == name)
        return false;

    const std::string old_name = m_name;
    m_name = name;

    Emit(&Rhi::IObjectCallback::OnObjectNameChanged, *this, old_name);
    return true;
}

} // namespace Methane::Graphics::Base