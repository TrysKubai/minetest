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
#include "guiLine.h"

GUILine::GUILine(
    gui::IGUIEnvironment *env, 
    gui::IGUIElement *parent, 
    s32 id,
    const core::rect<s32> &rectangle,
	const core::vector2di &start_pos, 
    const core::vector2di &end_pos, 
    const f32 &thickness,
    const video::SColor &color):
        gui::IGUIElement(
            gui::EGUIET_ELEMENT, 
            env, 
            parent, 
            id, 
            rectangle),
        _start_pos(start_pos),
        _end_pos(end_pos),
        _thickness(thickness),
        _color(color)
{
    if(parent)
    {
        const core::vector2di &parent_rect_pos = parent->getAbsolutePosition().UpperLeftCorner;
        _start_pos += parent_rect_pos;
        _end_pos += parent_rect_pos;
    }
}

void GUILine::draw()
{
    if (!IsVisible)
        return;

    video::IVideoDriver *driver = Environment->getVideoDriver();

    // Calculate angle and length of the line
    float dx = _end_pos.X - _start_pos.X;
    float dy = _end_pos.Y - _start_pos.Y;
    float angle = atan2(dy, dx);

    float offsetX = cos(angle + M_PI / 2) * (_thickness / 2);
    float offsetY = sin(angle + M_PI / 2) * (_thickness / 2);

    //* Note appearantly irrlicht modifications have removed the draw2DTriangle method,
    //* made draw2DRectangle axis aligned; and draw2DLine always draws a line of 1px width.
    //* So the remaining option is to draw the primitives manually.
    video::S3DVertex vertices[4] = {
        video::S3DVertex(_start_pos.X - offsetX, _start_pos.Y - offsetY, 0, 0, 0, 0, _color, 0, 0),
        video::S3DVertex(_end_pos.X - offsetX, _end_pos.Y - offsetY, 0, 0, 0, 0, _color, 0, 0),
        video::S3DVertex(_end_pos.X + offsetX, _end_pos.Y + offsetY, 0, 0, 0, 0, _color, 0, 0),
        video::S3DVertex(_start_pos.X + offsetX, _start_pos.Y + offsetY, 0, 0, 0, 0, _color, 0, 0)
    };

    u16 indices[] = {0, 1, 3, 3, 1, 2};
    driver->draw2DVertexPrimitiveList(vertices, 4, indices, 2, video::EVT_STANDARD, scene::EPT_TRIANGLES, video::EIT_16BIT);

    IGUIElement::draw();
}
