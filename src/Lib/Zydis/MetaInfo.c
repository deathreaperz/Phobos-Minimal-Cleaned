/***************************************************************************************************

  Zyan Disassembler Library (Zydis)

  Original Author : Florian Bernd

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.

***************************************************************************************************/

#include <Zydis/MetaInfo.h>
#include <Zycore/Types.h>

/* ============================================================================================== */
/* Enum strings                                                                                   */
/* ============================================================================================== */

#include <Generated/EnumInstructionCategory.inc>
#include <Generated/EnumISASet.inc>
#include <Generated/EnumISAExt.inc>

/* ============================================================================================== */
/* Exported functions                                                                             */
/* ============================================================================================== */

const char* ZydisCategoryGetString(ZydisInstructionCategory category)
{
	if ((ZyanUSize)category >= ZYAN_ARRAY_LENGTH(STR_INSTRUCTIONCATEGORY))
	{
		return ZYAN_NULL;
	}
	return STR_INSTRUCTIONCATEGORY[category];
}

const char* ZydisISASetGetString(ZydisISASet isa_set)
{
	if ((ZyanUSize)isa_set >= ZYAN_ARRAY_LENGTH(STR_ISASET))
	{
		return ZYAN_NULL;
	}
	return STR_ISASET[isa_set];
}

const char* ZydisISAExtGetString(ZydisISAExt isa_ext)
{
	if ((ZyanUSize)isa_ext >= ZYAN_ARRAY_LENGTH(STR_ISAEXT))
	{
		return ZYAN_NULL;
	}
	return STR_ISAEXT[isa_ext];
}

/* ============================================================================================== */