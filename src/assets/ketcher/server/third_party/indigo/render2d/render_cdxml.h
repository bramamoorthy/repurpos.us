/****************************************************************************
 * Copyright (C) 2009-2013 GGA Software Services LLC
 * 
 * This file is part of Indigo toolkit.
 * 
 * This file may be distributed and/or modified under the terms of the
 * GNU General Public License version 3 as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPL included in the
 * packaging of this file.
 * 
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 ***************************************************************************/

#ifndef __render_cdxml_h__
#define __render_cdxml_h__

namespace indigo {

class RenderParams;
class Molecule;
struct Vec2f;

class RenderParamCdxmlInterface {
public:
   static void render (RenderParams& params);
};

}

#endif // __render_cdxml_h__
