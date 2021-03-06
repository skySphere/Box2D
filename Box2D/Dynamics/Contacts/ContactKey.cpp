/*
 * Copyright (c) 2017 Louis Langholtz https://github.com/louis-langholtz/Box2D
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include <Box2D/Dynamics/Contacts/ContactKey.hpp>
#include <Box2D/Dynamics/Contacts/Contact.hpp>
#include <Box2D/Dynamics/Fixture.hpp>
#include <Box2D/Dynamics/FixtureProxy.hpp>

using namespace box2d;

ContactKey box2d::GetContactKey(const FixtureProxy& fpA, const FixtureProxy& fpB) noexcept
{
    return ContactKey{fpA.proxyId, fpB.proxyId};
}

ContactKey box2d::GetContactKey(const Fixture* fixtureA, ChildCounter childIndexA,
                                const Fixture* fixtureB, ChildCounter childIndexB) noexcept
{
    return GetContactKey(*fixtureA->GetProxy(childIndexA), *fixtureB->GetProxy(childIndexB));
}

ContactKey box2d::GetContactKey(const Contact& contact) noexcept
{
    return GetContactKey(contact.GetFixtureA(), contact.GetChildIndexA(),
                         contact.GetFixtureB(), contact.GetChildIndexB());
}
