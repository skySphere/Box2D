/*
 * Original work Copyright (c) 2006-2011 Erin Catto http://www.box2d.org
 * Modified work Copyright (c) 2017 Louis Langholtz https://github.com/louis-langholtz/Box2D
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef BodyAtty_hpp
#define BodyAtty_hpp

/// @file
/// Declaration of the BodyAtty class.

#include <Box2D/Dynamics/Body.hpp>
#include <Box2D/Dynamics/Fixture.hpp>
#include <Box2D/Dynamics/Joints/JointKey.hpp>

namespace box2d
{
    /// @brief Body attorney.
    ///
    /// @details This class uses the "attorney-client" idiom to control the granularity of
    ///   friend-based access to the Body class. This is meant to help preserve and enforce
    ///   the invariants of the Body class.
    ///
    /// @sa https://en.wikibooks.org/wiki/More_C++_Idioms/Friendship_and_the_Attorney-Client
    ///
    class BodyAtty
    {
    private:
        
        static Fixture* CreateFixture(Body& b, std::shared_ptr<const Shape> shape, const FixtureDef& def)
        {
            b.m_fixtures.emplace_front(&b, def, shape);
            return &b.m_fixtures.front();
        }
        
        static bool DestroyFixture(Body& b, Fixture* value)
        {
            auto prev = b.m_fixtures.before_begin();
            for (auto iter = b.m_fixtures.begin(); iter != b.m_fixtures.end(); ++iter)
            {
                if (&(*iter) == value)
                {
                    b.m_fixtures.erase_after(prev);
                    return true;
                }
                prev = iter;
            }
            return false;
        }

        static void ClearFixtures(Body& b, std::function<void(Fixture&)> callback)
        {
            while (!b.m_fixtures.empty())
            {
                auto& fixture = b.m_fixtures.front();
                callback(fixture);
                b.m_fixtures.pop_front();
            }
        }

        static void SetTypeFlags(Body& b, BodyType type) noexcept
        {
            b.m_flags &= ~(Body::e_impenetrableFlag|Body::e_velocityFlag|Body::e_accelerationFlag);
            b.m_flags |= Body::GetFlags(type);
            
            switch (type)
            {
                case BodyType::Dynamic:
                    break;
                case BodyType::Kinematic:
                    break;
                case BodyType::Static:
                    b.UnsetAwakeFlag();
                    b.m_underActiveTime = 0;
                    b.m_velocity = Velocity{Vec2_zero * MeterPerSecond, AngularVelocity{0}};
                    b.m_sweep.pos0 = b.m_sweep.pos1;
                    break;
            }
        }
        
        static void SetAwakeFlag(Body& b) noexcept
        {
            b.SetAwakeFlag();
        }
        
        static void SetMassDataDirty(Body& b) noexcept
        {
            b.SetMassDataDirty();
        }
        
        static bool Erase(Body& b, Contact* value)
        {
            return b.Erase(value);
        }
        
        static bool Erase(Body& b, Joint* value)
        {
            return b.Erase(value);
        }
        
        static bool Insert(Body& b, Joint* value)
        {
            return b.Insert(value);
        }
        
        static bool Insert(Body& b, Contact* value)
        {
            return b.Insert(value);
        }
        
        static void SetPosition0(Body& b, const Position value) noexcept
        {
            assert(b.IsSpeedable() || b.m_sweep.pos0 == value);
            b.m_sweep.pos0 = value;
        }
        
        /// Sets the body sweep's position 1 value.
        /// @note This sets what Body::GetWorldCenter returns.
        static void SetPosition1(Body& b, const Position value) noexcept
        {
            assert(b.IsSpeedable() || b.m_sweep.pos1 == value);
            b.m_sweep.pos1 = value;
        }
        
        static void ResetAlpha0(Body& b)
        {
            b.m_sweep.ResetAlpha0();
        }
        
        static void SetSweep(Body& b, const Sweep value) noexcept
        {
            assert(b.IsSpeedable() || value.pos0 == value.pos1);
            b.m_sweep = value;
        }
        
        /// Sets the body's transformation.
        /// @note This sets what Body::GetLocation returns.
        static void SetTransformation(Body& b, const Transformation value) noexcept
        {
            b.SetTransformation(value);
        }
        
        /// Sets the body's velocity.
        /// @note This sets what Body::GetVelocity returns.
        static void SetVelocity(Body& b, Velocity value) noexcept
        {
            b.m_velocity = value;
        }
        
        static void Advance0(Body& b, RealNum value) noexcept
        {
            // Note: Static bodies must **never** have different sweep position values.
            
            // Confirm bodies don't have different sweep positions to begin with...
            assert(b.IsSpeedable() || b.m_sweep.pos1 == b.m_sweep.pos0);
            
            b.m_sweep.Advance0(value);
            
            // Confirm bodies don't have different sweep positions to end with...
            assert(b.IsSpeedable() || b.m_sweep.pos1 == b.m_sweep.pos0);
        }
        
        static void Advance(Body& b, RealNum toi) noexcept
        {
            b.Advance(toi);
        }
        
        static void Restore(Body& b, const Sweep value) noexcept
        {
            BodyAtty::SetSweep(b, value);
            BodyAtty::SetTransformation(b, GetTransform1(value));
        }
        
        static void ClearJoints(Body& b, std::function<void(Joint&)> callback)
        {
            while (!b.m_joints.empty())
            {
                auto iter = b.m_joints.begin();
                const auto joint = iter->second;
                b.m_joints.erase(iter);
                callback(*joint);
            }
        }
        
        static void EraseContacts(Body& b, std::function<bool(Contact&)> callback)
        {
            const auto end = b.m_contacts.end();
            auto iter = b.m_contacts.begin();
            while (iter != end)
            {
                const auto contact = GetContactPtr(*iter);
                if (callback(*contact))
                {
                    const auto next = std::next(iter);
                    b.m_contacts.erase(iter);
                    iter = next;
                }
                else
                {
                    iter = std::next(iter);
                }
            }
        }
        
        friend class World;
    };

} // namespace box2d

#endif /* BodyAtty_hpp */
