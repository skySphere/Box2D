/*
* Original work Copyright (c) 2006-2007 Erin Catto http://www.box2d.org
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

#include <Box2D/Dynamics/Joints/Joint.hpp>
#include <Box2D/Dynamics/Joints/DistanceJoint.hpp>
#include <Box2D/Dynamics/Joints/WheelJoint.hpp>
#include <Box2D/Dynamics/Joints/MouseJoint.hpp>
#include <Box2D/Dynamics/Joints/RevoluteJoint.hpp>
#include <Box2D/Dynamics/Joints/PrismaticJoint.hpp>
#include <Box2D/Dynamics/Joints/PulleyJoint.hpp>
#include <Box2D/Dynamics/Joints/GearJoint.hpp>
#include <Box2D/Dynamics/Joints/WeldJoint.hpp>
#include <Box2D/Dynamics/Joints/FrictionJoint.hpp>
#include <Box2D/Dynamics/Joints/RopeJoint.hpp>
#include <Box2D/Dynamics/Joints/MotorJoint.hpp>
#include <Box2D/Dynamics/Body.hpp>
#include <Box2D/Dynamics/World.hpp>
#include <Box2D/Dynamics/Contacts/Contact.hpp>

#include <new>
#include <algorithm>

namespace box2d
{

namespace
{
    inline DistanceJoint* Create(const DistanceJointDef& def)
    {
        if (DistanceJoint::IsOkay(def))
        {
            return new DistanceJoint(def);
        }
        return nullptr;
    }
    
    inline MouseJoint* Create(const MouseJointDef& def)
    {
        if (MouseJoint::IsOkay(static_cast<const MouseJointDef&>(def)))
        {
            return new MouseJoint(def);
        }
        return nullptr;
    }
    
    inline PrismaticJoint* Create(const PrismaticJointDef& def)
    {
        if (PrismaticJoint::IsOkay(def))
        {
            return new PrismaticJoint(def);
        }
        return nullptr;
    }
    
    inline RevoluteJoint* Create(const RevoluteJointDef& def)
    {
        if (RevoluteJoint::IsOkay(def))
        {
            return new RevoluteJoint(def);
        }
        return nullptr;
    }
    
    inline PulleyJoint* Create(const PulleyJointDef& def)
    {
        if (PulleyJoint::IsOkay(def))
        {
            return new PulleyJoint(def);
        }
        return nullptr;
    }
    
    inline GearJoint* Create(const GearJointDef& def)
    {
        if (GearJoint::IsOkay(def))
        {
            return new GearJoint(def);
        }
        return nullptr;
    }
    
    inline WheelJoint* Create(const WheelJointDef& def)
    {
        if (WheelJoint::IsOkay(def))
        {
            return new WheelJoint(def);
        }
        return nullptr;
    }
    
    inline WeldJoint* Create(const WeldJointDef& def)
    {
        if (WeldJoint::IsOkay(def))
        {
            return new WeldJoint(def);
        }
        return nullptr;
    }
    
    inline FrictionJoint* Create(const FrictionJointDef& def)
    {
        if (FrictionJoint::IsOkay(def))
        {
            return new FrictionJoint(def);
        }
        return nullptr;
    }
    
    inline RopeJoint* Create(const RopeJointDef& def)
    {
        if (RopeJoint::IsOkay(def))
        {
            return new RopeJoint(def);
        }
        return nullptr;
    }
    
    inline MotorJoint* Create(const MotorJointDef& def)
    {
        if (MotorJoint::IsOkay(def))
        {
            return new MotorJoint(def);
        }
        return nullptr;
    }
    
} // anonymous namespace

Joint::FlagsType Joint::GetFlags(const JointDef& def) noexcept
{
    auto flags = Joint::FlagsType(0);
    if (def.collideConnected)
    {
        flags |= e_collideConnectedFlag;
    }
    return flags;
}

Joint* Joint::Create(const JointDef& def)
{
    switch (def.type)
    {
    case JointType::Distance:
        return box2d::Create(static_cast<const DistanceJointDef&>(def));
    case JointType::Mouse:
        return box2d::Create(static_cast<const MouseJointDef&>(def));
    case JointType::Prismatic:
        return box2d::Create(static_cast<const PrismaticJointDef&>(def));
    case JointType::Revolute:
        return box2d::Create(static_cast<const RevoluteJointDef&>(def));
    case JointType::Pulley:
        return box2d::Create(static_cast<const PulleyJointDef&>(def));
    case JointType::Gear:
        return box2d::Create(static_cast<const GearJointDef&>(def));
    case JointType::Wheel:
        return box2d::Create(static_cast<const WheelJointDef&>(def));
    case JointType::Weld:
        return box2d::Create(static_cast<const WeldJointDef&>(def));
    case JointType::Friction:
        return box2d::Create(static_cast<const FrictionJointDef&>(def));
    case JointType::Rope:
        return box2d::Create(static_cast<const RopeJointDef&>(def));
    case JointType::Motor:
        return box2d::Create(static_cast<const MotorJointDef&>(def));
    case JointType::Unknown:
        assert(false);
        break;
    default:
        break;
    }
    return nullptr;
}

void Joint::Destroy(const Joint* joint)
{
    delete joint;
}

bool Joint::IsOkay(const JointDef& def) noexcept
{
    if (def.bodyA == def.bodyB)
    {
        return false;
    }
    return true;
}

Joint::Joint(const JointDef& def):
    m_type{def.type}, m_bodyA{def.bodyA}, m_bodyB{def.bodyB},
    m_flags{GetFlags(def)}, m_userData{def.userData}
{
    // Intentionally empty.
}

bool IsEnabled(const Joint& j) noexcept
{
    return j.GetBodyA()->IsEnabled() && j.GetBodyB()->IsEnabled();
}

void SetAwake(Joint& j) noexcept
{
    j.GetBodyA()->SetAwake();
    j.GetBodyB()->SetAwake();
}

JointCounter GetWorldIndex(const Joint* joint)
{
    if (joint)
    {
        const auto bA = joint->GetBodyA();
        const auto bB = joint->GetBodyB();
        const auto world = bA? bA->GetWorld(): bB? bB->GetWorld(): static_cast<const World*>(nullptr);
        if (world)
        {
            auto i = JointCounter{0};
            const auto joints = world->GetJoints();
            const auto it = std::find_if(std::cbegin(joints), std::cend(joints), [&](const Joint *j) {
                return (j == joint) || (++i, false);
            });
            if (it != std::end(joints))
            {
                return i;
            }
        }
    }
    return JointCounter(-1);
}

void Set(JointDef& def, const Joint& joint) noexcept
{
    def.bodyA = joint.GetBodyA();
    def.bodyB = joint.GetBodyB();
    def.userData = joint.GetUserData();
    def.collideConnected = joint.GetCollideConnected();
}

} // namespace box2d
