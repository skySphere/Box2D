/*
* Original work Copyright (c) 2006-2012 Erin Catto http://www.box2d.org
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

#include <Box2D/Dynamics/Joints/MotorJoint.hpp>
#include <Box2D/Dynamics/Body.hpp>
#include <Box2D/Dynamics/StepConf.hpp>
#include <Box2D/Dynamics/Contacts/BodyConstraint.hpp>

using namespace box2d;

// Point-to-point constraint
// Cdot = v2 - v1
//      = v2 + cross(w2, r2) - v1 - cross(w1, r1)
// J = [-I -r1_skew I r2_skew ]
// Identity used:
// w k % (rx i + ry j) = w * (-ry i + rx j)

// Angle constraint
// Cdot = w2 - w1
// J = [0 0 -1 0 0 1]
// K = invI1 + invI2

MotorJointDef::MotorJointDef(Body* bA, Body* bB) noexcept:
    JointDef{JointType::Motor, bA, bB},
    linearOffset{GetLocalPoint(*bodyA, bodyB->GetLocation())},
    angularOffset{bodyB->GetAngle() - bodyA->GetAngle()}
{
}

MotorJoint::MotorJoint(const MotorJointDef& def):
    Joint(def),
    m_linearOffset(def.linearOffset),
    m_angularOffset(def.angularOffset),
    m_maxForce(def.maxForce),
    m_maxTorque(def.maxTorque),
    m_correctionFactor(def.correctionFactor)
{
    // Intentionally empty.
}

void MotorJoint::InitVelocityConstraints(BodyConstraintsMap& bodies, const StepConf& step, const ConstraintSolverConf&)
{
    auto& bodyConstraintA = At(bodies, GetBodyA());
    auto& bodyConstraintB = At(bodies, GetBodyB());

    const auto posA = bodyConstraintA->GetPosition();
    auto velA = bodyConstraintA->GetVelocity();

    const auto posB = bodyConstraintB->GetPosition();
    auto velB = bodyConstraintB->GetVelocity();

    const auto qA = UnitVec2(posA.angular);
    const auto qB = UnitVec2(posB.angular);

    // Compute the effective mass matrix.
    m_rA = Rotate(-bodyConstraintA->GetLocalCenter(), qA);
    m_rB = Rotate(-bodyConstraintB->GetLocalCenter(), qB);

    // J = [-I -r1_skew I r2_skew]
    //     [ 0       -1 0       1]
    // r_skew = [-ry; rx]

    // Matlab
    // K = [ mA+r1y^2*iA+mB+r2y^2*iB,  -r1y*iA*r1x-r2y*iB*r2x,          -r1y*iA-r2y*iB]
    //     [  -r1y*iA*r1x-r2y*iB*r2x, mA+r1x^2*iA+mB+r2x^2*iB,           r1x*iA+r2x*iB]
    //     [          -r1y*iA-r2y*iB,           r1x*iA+r2x*iB,                   iA+iB]

    const auto invMassA = bodyConstraintA->GetInvMass();
    const auto invMassB = bodyConstraintB->GetInvMass();
    const auto invRotInertiaA = bodyConstraintA->GetInvRotInertia();
    const auto invRotInertiaB = bodyConstraintB->GetInvRotInertia();

    {
        Mat22 K;
        const auto exx = InvMass{
            invMassA + invMassB +
            invRotInertiaA * Square(m_rA.y) / SquareRadian +
            invRotInertiaB * Square(m_rB.y) / SquareRadian
        };
        const auto exy = InvMass{
            -invRotInertiaA * m_rA.x * m_rA.y / SquareRadian +
            -invRotInertiaB * m_rB.x * m_rB.y / SquareRadian
        };
        const auto eyy = InvMass{
            invMassA + invMassB +
            invRotInertiaA * Square(m_rA.x) / SquareRadian +
            invRotInertiaB * Square(m_rB.x) / SquareRadian
        };
        K.ex.x = StripUnit(exx);
        K.ex.y = StripUnit(exy);
        K.ey.x = K.ex.y;
        K.ey.y = StripUnit(eyy);
        m_linearMass = Invert(K);
    }
    
    const auto invRotInertia = invRotInertiaA + invRotInertiaB;
    m_angularMass = (invRotInertia > InvRotInertia{0})? RotInertia{Real{1} / invRotInertia}: RotInertia{0};
    
    m_linearError = posB.linear + m_rB - posA.linear - m_rA - Rotate(m_linearOffset, qA);
    m_angularError = posB.angular - posA.angular - m_angularOffset;

    if (step.doWarmStart)
    {
        // Scale impulses to support a variable time step.
        m_linearImpulse *= step.dtRatio;
        m_angularImpulse *= step.dtRatio;

        const auto P = m_linearImpulse;
        // L * M * L T^-1 / QP is: L^2 M T^-1 QP^-1 which is: AngularMomentum.
        const auto crossAP = AngularMomentum{Cross(m_rA, P) / Radian};
        const auto crossBP = AngularMomentum{Cross(m_rB, P) / Radian}; // L * M * L T^-1 is: L^2 M T^-1

        velA -= Velocity{invMassA * P, invRotInertiaA * (crossAP + m_angularImpulse)};
        velB += Velocity{invMassB * P, invRotInertiaB * (crossBP + m_angularImpulse)};
    }
    else
    {
        m_linearImpulse = Momentum2D{0, 0};
        m_angularImpulse = AngularMomentum{0};
    }

    bodyConstraintA->SetVelocity(velA);
    bodyConstraintB->SetVelocity(velB);
}

bool MotorJoint::SolveVelocityConstraints(BodyConstraintsMap& bodies, const StepConf& step)
{
    auto& bodyConstraintA = At(bodies, GetBodyA());
    auto& bodyConstraintB = At(bodies, GetBodyB());

    auto velA = bodyConstraintA->GetVelocity();
    auto velB = bodyConstraintB->GetVelocity();

    const auto invMassA = bodyConstraintA->GetInvMass();
    const auto invMassB = bodyConstraintB->GetInvMass();
    const auto invRotInertiaA = bodyConstraintA->GetInvRotInertia();
    const auto invRotInertiaB = bodyConstraintB->GetInvRotInertia();

    const auto h = step.GetTime();
    const auto inv_h = step.GetInvTime();

    auto solved = true;

    // Solve angular friction
    {
        const auto Cdot = AngularVelocity{(velB.angular - velA.angular) + inv_h * m_correctionFactor * m_angularError};
        const auto angularImpulse = AngularMomentum{-m_angularMass * Cdot};

        const auto oldAngularImpulse = m_angularImpulse;
        const auto maxAngularImpulse = h * m_maxTorque;
        m_angularImpulse = Clamp(m_angularImpulse + angularImpulse, -maxAngularImpulse, maxAngularImpulse);
        const auto incAngularImpulse = m_angularImpulse - oldAngularImpulse;

        if (incAngularImpulse != AngularMomentum(0))
        {
            solved = false;
        }
        velA.angular -= invRotInertiaA * incAngularImpulse;
        velB.angular += invRotInertiaB * incAngularImpulse;
    }

    // Solve linear friction
    {
        const auto vb = LinearVelocity2D{velB.linear + (GetRevPerpendicular(m_rB) * (velB.angular / Radian))};
        const auto va = LinearVelocity2D{velA.linear - (GetRevPerpendicular(m_rA) * (velA.angular / Radian))};

        const auto Cdot = LinearVelocity2D{(vb - va) + inv_h * m_correctionFactor * m_linearError};

        const auto ulImp = -Transform(GetVec2(Cdot), m_linearMass);
        const auto impulse = Momentum2D{
            ulImp.GetX() * Kilogram * MeterPerSecond,
            ulImp.GetY() * Kilogram * MeterPerSecond
        };
        const auto oldImpulse = m_linearImpulse;
        m_linearImpulse += impulse;

        const auto maxImpulse = h * m_maxForce;

        if (GetLengthSquared(m_linearImpulse) > Square(maxImpulse))
        {
            m_linearImpulse = GetUnitVector(m_linearImpulse, UnitVec2::GetZero()) * maxImpulse;
        }

        const auto incImpulse = m_linearImpulse - oldImpulse;
        const auto angImpulseA = AngularMomentum{Cross(m_rA, incImpulse) / Radian};
        const auto angImpulseB = AngularMomentum{Cross(m_rB, incImpulse) / Radian};

        if (incImpulse != Momentum2D{0, 0})
        {
            solved = false;
        }

        velA -= Velocity{invMassA * incImpulse, invRotInertiaA * angImpulseA};
        velB += Velocity{invMassB * incImpulse, invRotInertiaB * angImpulseB};
    }

    bodyConstraintA->SetVelocity(velA);
    bodyConstraintB->SetVelocity(velB);
    
    return solved;
}

bool MotorJoint::SolvePositionConstraints(BodyConstraintsMap& bodies, const ConstraintSolverConf& conf) const
{
    NOT_USED(bodies);
    NOT_USED(conf);

    return true;
}

Length2D MotorJoint::GetAnchorA() const
{
    return GetBodyA()->GetLocation();
}

Length2D MotorJoint::GetAnchorB() const
{
    return GetBodyB()->GetLocation();
}

Force2D MotorJoint::GetReactionForce(Frequency inv_dt) const
{
    return inv_dt * m_linearImpulse;
}

Torque MotorJoint::GetReactionTorque(Frequency inv_dt) const
{
    return inv_dt * m_angularImpulse;
}

void MotorJoint::SetMaxForce(Force force)
{
    assert(IsValid(force) && (force >= Force{0}));
    m_maxForce = force;
}

Force MotorJoint::GetMaxForce() const
{
    return m_maxForce;
}

void MotorJoint::SetMaxTorque(Torque torque)
{
    assert(IsValid(torque) && (torque >= Torque{0}));
    m_maxTorque = torque;
}

Torque MotorJoint::GetMaxTorque() const
{
    return m_maxTorque;
}

void MotorJoint::SetCorrectionFactor(Real factor)
{
    assert(IsValid(factor) && (0 <= factor) && (factor <= Real{1}));
    m_correctionFactor = factor;
}

Real MotorJoint::GetCorrectionFactor() const
{
    return m_correctionFactor;
}

void MotorJoint::SetLinearOffset(const Length2D linearOffset)
{
    if ((linearOffset.x != m_linearOffset.x) || (linearOffset.y != m_linearOffset.y))
    {
        m_linearOffset = linearOffset;

        GetBodyA()->SetAwake();
        GetBodyB()->SetAwake();
    }
}

const Length2D MotorJoint::GetLinearOffset() const
{
    return m_linearOffset;
}

void MotorJoint::SetAngularOffset(Angle angularOffset)
{
    if (angularOffset != m_angularOffset)
    {
        m_angularOffset = angularOffset;

        GetBodyA()->SetAwake();
        GetBodyB()->SetAwake();
    }
}

Angle MotorJoint::GetAngularOffset() const
{
    return m_angularOffset;
}

MotorJointDef box2d::GetMotorJointDef(const MotorJoint& joint) noexcept
{
    auto def = MotorJointDef{};
    
    Set(def, joint);
    
    def.linearOffset = joint.GetLinearOffset();
    def.angularOffset = joint.GetAngularOffset();
    def.maxForce = joint.GetMaxForce();
    def.maxTorque = joint.GetMaxTorque();
    def.correctionFactor = joint.GetCorrectionFactor();
    
    return def;
}
