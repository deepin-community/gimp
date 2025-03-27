/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/* This program generates the header file "huffman.h".
 * huffman.h will contain an array of Huffnode structs which make up two
 * Huffman code trees for black and white pixels, respectively.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "generate-huffman.h"

#define ARR_SIZE(a) (sizeof a / sizeof a[0])

struct Node
{
  int l;
  int r;
  int value;
  int leaf;
  int makeup;
};

static int         nnodes = 0;
static struct Node nodebuffer[416];

static int blackroot = -1;
static int whiteroot = -1;

static void s_buildtree      (void);
static void add_node         (int        *idx,
                              const char *bits,
                              int         value,
                              int         makeup);
static void find_empty_nodes (int         idx,
                              int         depth);

int
main (int argc, char *argv[])
{
  int          i;
  struct Node *n = nodebuffer;
  FILE        *file;

  if (argc == 2)
    {
      if (! strcmp (argv[1], "-"))
        {
          file = stdout;
        }
      else if (! (file = fopen (argv[1], "w")))
        {
          perror (argv[1]);
          return 1;
        }
    }
  else
    {
      fprintf (stderr, "Need filespec to save generated huffman header!\n");
      return 1;
    }

  fprintf (file, "/* This file was auto-generated by\n * %s\n */\n\n", argv[0]);

  s_buildtree ();

  /* sanity checks */
  find_empty_nodes (blackroot, 0);
  find_empty_nodes (whiteroot, 0);
  if (nnodes != 416)
    {
      fprintf (stderr, "*** have %d nodes, but should have 416.\n", nnodes);
      return 1;
    }

  fprintf (file, "static const int blackroot = %d;\n", blackroot);
  fprintf (file, "static const int whiteroot = %d;\n\n", whiteroot);
  fprintf (file, "static const struct Huffnode nodebuffer[] = {\n  ");
  for (i = 0; i < ARR_SIZE (nodebuffer); i++)
    {
      if ((i + 1) % 2 == 0 && i < ARR_SIZE (nodebuffer) - 1)
        {
          fprintf (file, "{ %3d, %3d, %4d, %2d, %2d },\n  ", n[i].l, n[i].r,
                   n[i].value, n[i].leaf, n[i].makeup);
        }
      else
        {
          fprintf (file, "{ %3d, %3d, %4d, %2d, %2d }, ", n[i].l, n[i].r,
                   n[i].value, n[i].leaf, n[i].makeup);
        }
    }
  fprintf (file, "\n};\n");
  return 0;
}

static void
s_buildtree (void)
{
  int i;

  memset (nodebuffer, 0, sizeof nodebuffer);

  for (i = 0; i < ARR_SIZE (huff_term_black); i++)
    add_node (&blackroot, huff_term_black[i].bits, huff_term_black[i].number, 0);

  for (i = 0; i < ARR_SIZE (huff_makeup_black); i++)
    add_node (&blackroot, huff_makeup_black[i].bits, huff_makeup_black[i].number, 1);

  for (i = 0; i < ARR_SIZE (huff_term_white); i++)
    add_node (&whiteroot, huff_term_white[i].bits, huff_term_white[i].number, 0);

  for (i = 0; i < ARR_SIZE (huff_makeup_white); i++)
    add_node (&whiteroot, huff_makeup_white[i].bits, huff_makeup_white[i].number, 1);
}

static void
add_node (int *idx, const char *bits, int value, int makeup)
{
  if (*idx == -1)
    {
      if (nnodes >= ARR_SIZE (nodebuffer))
        {
          fprintf (stderr, "*** too many nodes (have %d, max is %d)\n", nnodes,
                   (int) ARR_SIZE (nodebuffer));
          exit (1);
        }
      *idx               = nnodes++;
      nodebuffer[*idx].l = -1;
      nodebuffer[*idx].r = -1;
    }

  if (! *bits)
    {
      /* we are on the final bit of the sequence */
      nodebuffer[*idx].value  = value;
      nodebuffer[*idx].leaf   = 1;
      nodebuffer[*idx].makeup = makeup;
    }
  else
    {
      switch (*bits)
        {
        case '0':
          add_node (&nodebuffer[*idx].l, bits + 1, value, makeup);
          break;
        case '1':
          add_node (&nodebuffer[*idx].r, bits + 1, value, makeup);
          break;
        }
    }
}

static void
find_empty_nodes (int idx, int depth)
{
  static char        str[30];
  const struct Node *node;

  if (depth >= (int) sizeof str)
    {
      fprintf (stderr, "*** panic!, str[] is too short\n");
      exit (1);
    }

  str[depth] = 0;

  node = &nodebuffer[idx];
  if (! node->leaf)
    {
      if (! (node->l != -1 && node->r != -1))
        {
          if (node->r != -1 && ! strcmp ("0000000", str))
            {
              ; /* ok, that's eol */
            }
          else
            {
              fprintf (stderr, "*** node %s not full. l=%d r=%d\n", str, node->l, node->r);
              exit (1);
            }
        }
      if (node->l != -1)
        {
          str[depth] = '0';
          find_empty_nodes (node->l, depth + 1);
        }
      if (node->r != -1)
        {
          str[depth] = '1';
          find_empty_nodes (node->r, depth + 1);
        }
    }
}
