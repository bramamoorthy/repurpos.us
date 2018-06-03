/****************************************************************************
 * Copyright (C) 2009-2012 GGA Software Services LLC
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

#include "base_cpp/output.h"
#include "molecule/molecule_cdxml_saver.h"
#include "molecule/molecule.h"
#include "molecule/elements.h"
#include "base_cpp/locale_guard.h"

using namespace indigo;

IMPL_ERROR(MoleculeCdxmlSaver, "molecule CMXML saver");

MoleculeCdxmlSaver::MoleculeCdxmlSaver (Output &output) : _output(output)
{
   _bond_length = 30;
   _max_page_height = 64;
   _pages_height = 1;
}

float MoleculeCdxmlSaver::pageHeight () const
{
   return _max_page_height;
}

float MoleculeCdxmlSaver::textLineHeight () const
{
   return 12.75f / _bond_length;
}

void MoleculeCdxmlSaver::beginDocument (Bounds *bounds)
{
   _output.printf("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
   _output.printf("<!DOCTYPE CDXML SYSTEM \"http://www.cambridgesoft.com/xml/cdxml.dtd\" >\n");
      
   _output.printf("<CDXML _bond_length=\"%f\"", _bond_length);
   if (bounds != NULL)
   {
      // Generate MacPrintInfo according to the size
      // http://www.cambridgesoft.com/services/documentation/sdk/chemdraw/cdx/properties/MacPrintInfo.htm

      int dpi_logical = 72;
      int dpi_print = 600;

      float x_inch = bounds->max.x * _bond_length / dpi_logical + 1;
      float y_inch = bounds->max.y * _bond_length / dpi_logical + 1;

      int width = (int)(x_inch * dpi_print);
      int height = (int)(y_inch * dpi_print);

      // Add 1 to compensate margins = 36 points = 0.5 inches
      int max_height = (int)((_max_page_height * _bond_length / dpi_logical + 1) * dpi_print);
      if (height > max_height)
      {
         _pages_height = (int)ceil((float)height / max_height);
         height = max_height;
      }

      int mac_print_info[60] = {0};
      mac_print_info[0] = 3;  // magic number
      mac_print_info[2] = dpi_print;
      mac_print_info[3] = dpi_print;

      mac_print_info[6] = height;
      mac_print_info[7] = width;

      mac_print_info[10] = height;
      mac_print_info[11] = width;

      mac_print_info[12] = 871; // magic number

      mac_print_info[13] = height / 5; // magic scaling coeffient
      mac_print_info[14] = width / 5;

      mac_print_info[24] = 100; // horizontal scale, in percent
      mac_print_info[25] = 100; // Vertical scale, in percent

      _output.printf(" PrintMargins=\"36 36 36 36\"\n");
      _output.printf(" MacPrintInfo=\"");
      for (int i = 0; i < NELEM(mac_print_info); i++)
      {
         _output.printf("%04hx", (unsigned short)mac_print_info[i]);
      }
      
      _output.printf("\"\n");
   }
   _output.printf(">\n");
}

void MoleculeCdxmlSaver::beginPage (Bounds *bounds)
{
   _output.printf("<page ");
   _output.printf("HeightPages=\"%d\" WidthPages=\"1\"", _pages_height);
   _output.printf(">\n");
}

void MoleculeCdxmlSaver::saveMoleculeFragment (Molecule &mol, const Vec2f &offset, float structure_scale)
{
   float scale = structure_scale * _bond_length;

   LocaleGuard locale_guard;

   _output.printf("<fragment>\n");

   bool have_hyz = mol.have_xyz;

   Vec2f min_coord, max_coord;

   if (mol.vertexCount() > 0)
   {
      for (int i = mol.vertexBegin(); i != mol.vertexEnd(); i = mol.vertexNext(i))
      {
         if (mol.isRSite(i))
            throw Error("R-sites are not supported");

         int atom_number = mol.getAtomNumber(i);

         _output.printf("    <n id=\"%d\" Element=\"%d\"", i + 1, atom_number);

         if (mol.getAtomIsotope(i) != 0)
         {
            _output.printf(" Isotope=\"%d\"", mol.getAtomIsotope(i));
         }

         int charge = mol.getAtomCharge(i);
         if (charge != 0)
            _output.printf(" Charge=\"%d\"", charge);

         int radical = mol.getAtomRadical_NoThrow(i, 0);
         if (radical != 0)
         {
            const char *radical_str = NULL;
            if (radical == RADICAL_DOUBLET)
               radical_str = "Doublet";
            else if (radical == RADICAL_SINGLET)
               radical_str = "Singlet";
            else
               throw Error("Radical type %d is not supported", radical);

            _output.printf(" Radical=\"%s\"", radical_str);
         }

         if (Molecule::shouldWriteHCount(mol, i))
         {
            int hcount;

            try
            {
               hcount = mol.getAtomTotalH(i);
            }
            catch (Exception &)
            {
               hcount = -1;
            }

            if (hcount >= 0)
               _output.printf(" NumHydrogens=\"%d\"", hcount);
         }

         Vec3f pos3 = mol.getAtomXyz(i);
         Vec2f pos(pos3.x, pos3.y);

         pos.add(offset);
         if (i == mol.vertexBegin())
            min_coord = max_coord = pos;
         else
         {
            min_coord.min(pos);
            max_coord.max(pos);
         }

         pos.scale(scale);
         if (have_hyz)
         {
            _output.printf("\n         p=\"%f %f\"", pos.x, -pos.y);
         }
         else
         {
            if (mol.stereocenters.getType(i) > MoleculeStereocenters::ATOM_ANY)
            {
               _output.printf(" Geometry=\"Tetrahedral\"");

               const int *pyramid = mol.stereocenters.getPyramid(i);
               // 0 means atom absence
               _output.printf(" BondOrdering=\"%d %d %d %d\"", 
                  pyramid[0] + 1, pyramid[1] + 1, pyramid[2] + 1, pyramid[3] + 1); 
            }
         }

         if (mol.isPseudoAtom(i))
         {
            throw Error("Pseudoatoms are not supported yet");
         }

         if (mol.getVertex(i).degree() == 0 && atom_number == ELEM_C && charge == 0 && radical == 0)
         {
            _output.printf(">\n");
            // Add explicit text label
            _output.printf("<t p=\"%f %f\" Justification=\"Center\"><s font=\"3\" size=\"10\" face=\"96\">CH4</s></t>\n", pos.x, -pos.y);
            _output.printf("</n>\n");
         }
         else
         {
            _output.printf("/>\n");
         }
      }
   }

   if (mol.edgeCount() > 0)
   {
      for (int i = mol.edgeBegin(); i != mol.edgeEnd(); i = mol.edgeNext(i))
      {
         const Edge &edge = mol.getEdge(i);

         _output.printf("    <b B=\"%d\" E=\"%d\"", edge.beg + 1, edge.end + 1);

         int order = mol.getBondOrder(i);

         if (order == BOND_DOUBLE || order == BOND_TRIPLE)
            _output.printf(" Order=\"%d\"", order);
         else if (order == BOND_AROMATIC)
            _output.printf(" Order=\"1.5\"");
         else
            ; // Do not write single bond order

         int dir = mol.getBondDirection(i);
         int parity = mol.cis_trans.getParity(i);

         if (mol.have_xyz && (dir == BOND_UP || dir == BOND_DOWN))
         {
            _output.printf(" Display=\"%s\"",
                    (dir == BOND_UP) ? "WedgeBegin" : "WedgedHashBegin");
         }
         else if (!mol.have_xyz && parity != 0)
         {
            const int *subst = mol.cis_trans.getSubstituents(i);

            int s3 = subst[2] + 1, s4  = subst[3] + 1;
            if (parity == MoleculeCisTrans::TRANS)
            {
               int tmp;
               __swap(s3, s4, tmp);
            }
            _output.printf(" BondCircularOrdering=\"%d %d %d %d\"",
                    subst[0] + 1, subst[1] + 1, s3, s4);
         }

         _output.printf("/>\n");
      }
   }

   if (mol.isChrial())
   {
      Vec2f chiral_pos(max_coord.x, max_coord.y);
      Vec2f bbox(scale * chiral_pos.x, -scale * chiral_pos.y);
      _output.printf("<graphic BoundingBox=\"%f %f %f %f\" GraphicType=\"Symbol\" SymbolType=\"Absolute\" FrameType=\"None\">\n", 
         bbox.x, bbox.y, bbox.x, bbox.y);
      addText(chiral_pos, "Chiral");
      _output.printf("</graphic>\n");
   }

   _output.printf("</fragment>\n");
}

void MoleculeCdxmlSaver::addText (const Vec2f &pos, const char *text)
{
   addText(pos, text, "Center");
}

void MoleculeCdxmlSaver::addText (const Vec2f &pos, const char *text, const char *alignment)
{
   _output.printf("<t p=\"%f %f\" Justification=\"%s\"><s>%s</s></t>\n", _bond_length * pos.x, -_bond_length * pos.y, alignment, text);
}

void MoleculeCdxmlSaver::endPage ()
{
   _output.printf("</page>\n");
}

void MoleculeCdxmlSaver::endDocument ()
{
   _output.printf("</CDXML>\n");
}


void MoleculeCdxmlSaver::saveMolecule (Molecule &mol)
{
   Vec3f min_coord, max_coord;
   if (mol.have_xyz)
   {
      for (int i = mol.vertexBegin(); i != mol.vertexEnd(); i = mol.vertexNext(i))
      {
         Vec3f &pos = mol.getAtomXyz(i);
         if (i == mol.vertexBegin())
            min_coord = max_coord = pos;
         else
         {
            min_coord.min(pos);
            max_coord.max(pos);
         }
      }

      // Add margins
      max_coord.add(Vec3f(1, 1, 1));
      min_coord.sub(Vec3f(1, 1, 1));
   }
   else
   {
      min_coord.set(0, 0, 0);
      max_coord.set(0, 0, 0);
   }

   beginDocument(NULL);
   beginPage(NULL);

   Vec2f offset(-min_coord.x, -max_coord.y);

   saveMoleculeFragment(mol, offset, 1);
   endPage();
   endDocument();
}
