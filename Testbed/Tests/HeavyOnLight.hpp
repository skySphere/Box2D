/*
 * Original work Copyright (c) 2008-2014 Erin Catto http://www.box2d.org
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

#ifndef HEAVY_ON_LIGHT_H
#define HEAVY_ON_LIGHT_H

#include "../Framework/Test.hpp"

namespace box2d {

class HeavyOnLight : public Test
{
public:
    
    HeavyOnLight()
    {
        const auto ground = m_world->CreateBody();
        ground->CreateFixture(std::make_shared<EdgeShape>(Vec2(-40.0f, 0.0f) * Meter, Vec2(40.0f, 0.0f) * Meter));
        
        auto conf = DiskShape::Conf{};

        BodyDef bd;
        bd.type = BodyType::Dynamic;

        bd.position = Vec2(0.0f, 0.5f) * Meter;
        const auto body1 = m_world->CreateBody(bd);
        conf.vertexRadius = Real{0.5f} * Meter;
        conf.density = Real(10) * KilogramPerSquareMeter;
        body1->CreateFixture(std::make_shared<DiskShape>(conf));
        
        bd.position = Vec2(0.0f, 6.0f) * Meter;
        const auto body2 = m_world->CreateBody(bd);
        conf.vertexRadius = Real{5.0f} * Meter;
        conf.density = Real(10) * KilogramPerSquareMeter;
        m_top = body2->CreateFixture(std::make_shared<DiskShape>(conf));
    }

    void ChangeDensity(Density change)
    {
        const auto oldDensity = m_top->GetShape()->GetDensity();
        const auto newDensity = std::max(oldDensity + change, KilogramPerSquareMeter);
        if (newDensity != oldDensity)
        {
            const auto wasSelected = GetSelectedFixture() == m_top;
            const auto body = m_top->GetBody();
            body->DestroyFixture(m_top);
            auto conf = DiskShape::Conf{};
            conf.vertexRadius = Real{5.0f} * Meter;
            conf.density = newDensity;
            m_top = body->CreateFixture(std::make_shared<DiskShape>(conf));
            if (wasSelected)
            {
                SetSelectedFixture(m_top);
            }
        }
    }

    void KeyboardDown(Key key) override
    {
        switch (key)
        {
            case Key_Add:
                ChangeDensity(+KilogramPerSquareMeter);
                break;
            case Key_Subtract:
                ChangeDensity(-KilogramPerSquareMeter);
                break;
            default:
                break;
        }
    }

    void PostStep(const Settings&, Drawer& drawer) override
    {
        drawer.DrawString(5, m_textLine,
                          "Press '+'/'-' to increase/decrease density of top shape (%f kg/m^2)",
                          double(Real{m_top->GetShape()->GetDensity() / KilogramPerSquareMeter}));
        m_textLine += DRAW_STRING_NEW_LINE;
    }

    Fixture* m_top = nullptr;
};

} // namespace box2d

#endif
