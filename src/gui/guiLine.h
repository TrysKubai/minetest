/* 
Three Cubes
(C) 2023

This file is part of Three Cubes Runtime, which itself is a modification of 
Minetest, a free and open-source game distributed under the 
GNU Lesser General Public License (LGPL). This file has been added or modified
from the original source in 2023.

Minetest is free software: you can redistribute it and/or modify it under the terms of the
GNU Lesser General Public License as published by the Free Software Foundation, either
version 2.1 of the License, or (at your option) any later version.

Minetest is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.

The source code is made available as per requirements of the LGPL license.
You have a right to use it as you see fit whithin the rights granted by LGPL 2.1.

For more info see https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html

*/

#pragma once

#include <vector>
#include <array>
#include "irrlichttypes_extrabloated.h"

class GUILine : public gui::IGUIElement
{
private:
    core::vector2di _start_pos;
    core::vector2di _end_pos;
    f32 _thickness;
    video::SColor _color;
public:
    GUILine(gui::IGUIEnvironment *env, 
        gui::IGUIElement *parent, 
        s32 id,
        const core::rect<s32> &rectangle,
		const core::vector2di &start_pos,
        const core::vector2di &end_pos,
        const f32 &thickness,
		const video::SColor &color
        );

    virtual void draw() override;
};
