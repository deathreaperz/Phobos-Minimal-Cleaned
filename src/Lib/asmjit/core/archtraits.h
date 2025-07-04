// This file is part of AsmJit project <https://asmjit.com>
//
// See <asmjit/core.h> or LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#ifndef ASMJIT_CORE_ARCHTRAITS_H_INCLUDED
#define ASMJIT_CORE_ARCHTRAITS_H_INCLUDED

#include "../core/operand.h"
#include "../core/support.h"
#include "../core/type.h"

ASMJIT_BEGIN_NAMESPACE

//! \addtogroup asmjit_core
//! \{
//! Instruction set architecture (ISA).
enum class Arch : uint8_t
{
	//! Unknown or uninitialized ISA.
	kUnknown = 0,

	//! 32-bit X86 ISA.
	kX86 = 1,
	//! 64-bit X86 ISA also known as X64, X86_64, and AMD64.
	kX64 = 2,

	//! 32-bit RISC-V ISA.
	kRISCV32 = 3,
	//! 64-bit RISC-V ISA.
	kRISCV64 = 4,

	//! 32-bit ARM ISA (little endian).
	kARM = 5,
	//! 64-bit ARM ISA in (little endian).
	kAArch64 = 6,
	//! 32-bit ARM ISA in Thumb mode (little endian).
	kThumb = 7,

	// 8 is not used at the moment, even numbers are 64-bit architectures.

	//! 32-bit MIPS ISA in (little endian).
	kMIPS32_LE = 9,
	//! 64-bit MIPS ISA in (little endian).
	kMIPS64_LE = 10,

	//! 32-bit ARM ISA (big endian).
	kARM_BE = 11,
	//! 64-bit ARM ISA in (big endian).
	kAArch64_BE = 12,
	//! 32-bit ARM ISA in Thumb mode (big endian).
	kThumb_BE = 13,

	// 14 is not used at the moment, even numbers are 64-bit architectures.

	//! 32-bit MIPS ISA in (big endian).
	kMIPS32_BE = 15,
	//! 64-bit MIPS ISA in (big endian).
	kMIPS64_BE = 16,

	//! Maximum value of `Arch`.
	kMaxValue = kMIPS64_BE,

	//! Mask used by 32-bit ISAs (odd are 32-bit, even are 64-bit).
	k32BitMask = 0x01,
	//! First big-endian architecture.
	kBigEndian = kARM_BE,

	//! ISA detected at compile-time (ISA of the host).
	kHost =
#if defined(_DOXYGEN)
	DETECTED_AT_COMPILE_TIME
#else
	ASMJIT_ARCH_X86 == 32 ? kX86 :
	ASMJIT_ARCH_X86 == 64 ? kX64 :

	ASMJIT_ARCH_RISCV == 32 ? kRISCV32 :
	ASMJIT_ARCH_RISCV == 64 ? kRISCV64 :

	ASMJIT_ARCH_ARM == 32 && ASMJIT_ARCH_LE ? kARM :
	ASMJIT_ARCH_ARM == 32 && ASMJIT_ARCH_BE ? kARM_BE :
	ASMJIT_ARCH_ARM == 64 && ASMJIT_ARCH_LE ? kAArch64 :
	ASMJIT_ARCH_ARM == 64 && ASMJIT_ARCH_BE ? kAArch64_BE :

	ASMJIT_ARCH_MIPS == 32 && ASMJIT_ARCH_LE ? kMIPS32_LE :
	ASMJIT_ARCH_MIPS == 32 && ASMJIT_ARCH_BE ? kMIPS32_BE :
	ASMJIT_ARCH_MIPS == 64 && ASMJIT_ARCH_LE ? kMIPS64_LE :
	ASMJIT_ARCH_MIPS == 64 && ASMJIT_ARCH_BE ? kMIPS64_BE :

	kUnknown
#endif
};

//! Sub-architecture.
enum class SubArch : uint8_t
{
	//! Unknown or uninitialized architecture sub-type.
	kUnknown = 0,

	//! Maximum value of `SubArch`.
	kMaxValue = kUnknown,

	//! Sub-architecture detected at compile-time (sub-architecture of the host).
	kHost =
#if defined(_DOXYGEN)
	DETECTED_AT_COMPILE_TIME
#else
	kUnknown
#endif
};

//! Identifier used to represent names of different data types across architectures.
enum class ArchTypeNameId : uint8_t
{
	//! Describes 'db' (X86|X86_64 convention, always 8-bit quantity).
	kDB = 0,
	//! Describes 'dw' (X86|X86_64 convention, always 16-bit word).
	kDW,
	//! Describes 'dd' (X86|X86_64 convention, always 32-bit word).
	kDD,
	//! Describes 'dq' (X86|X86_64 convention, always 64-bit word).
	kDQ,
	//! Describes 'byte' (always 8-bit quantity).
	kByte,
	//! Describes 'half' (most likely 16-bit word).
	kHalf,
	//! Describes 'word' (either 16-bit or 32-bit word).
	kWord,
	//! Describes 'hword' (most likely 16-bit word).
	kHWord,
	//! Describes 'dword' (either 32-bit or 64-bit word).
	kDWord,
	//! Describes 'qword' (64-bit word).
	kQWord,
	//! Describes 'xword' (64-bit word).
	kXWord,
	//! Describes 'short' (always 16-bit word).
	kShort,
	//! Describes 'long' (most likely 32-bit word).
	kLong,
	//! Describes 'quad' (64-bit word).
	kQuad,

	//! Maximum value of `ArchTypeNameId`.
	kMaxValue = kQuad
};

//! Instruction feature hints for each register group provided by \ref ArchTraits.
//!
//! Instruction feature hints describe miscellaneous instructions provided by the architecture that can be used by
//! register allocator to make certain things simpler - like register swaps or emitting register push/pop sequences.
//!
//! \remarks Instruction feature hints are only defined for register groups that can be used with \ref
//! asmjit_compiler infrastructure. Register groups that are not managed by Compiler are not provided by
//! \ref ArchTraits and cannot be queried.
enum class InstHints : uint8_t
{
	//! No feature hints.
	kNoHints = 0,

	//! Architecture supports a register swap by using a single instruction.
	kRegSwap = 0x01u,
	//! Architecture provides push/pop instructions.
	kPushPop = 0x02u
};
ASMJIT_DEFINE_ENUM_FLAGS(InstHints)

//! Architecture traits used by Function API and Compiler's register allocator.
struct ArchTraits
{
	//! \name Members
	//! \{
	//! Stack pointer register id.
	uint8_t _spRegId;
	//! Frame pointer register id.
	uint8_t _fpRegId;
	//! Link register id.
	uint8_t _linkRegId;
	//! Instruction pointer (or program counter) register id, if accessible.
	uint8_t _pcRegId;

	// Reserved.
	uint8_t _reserved[3];
	//! Hardware stack alignment requirement.
	uint8_t _hwStackAlignment;

	//! Minimum addressable offset on stack guaranteed for all instructions.
	uint32_t _minStackOffset;
	//! Maximum addressable offset on stack depending on specific instruction.
	uint32_t _maxStackOffset;

	//! Bit-mask indexed by \ref RegType that describes, which register types are supported by the ISA.
	uint32_t _supportedRegTypes;

	//! Flags for each virtual register group.
	Support::Array<InstHints, Globals::kNumVirtGroups> _instHints;

	//! Maps scalar TypeId values (from TypeId::_kIdBaseStart) to register types, see \ref TypeId.
	Support::Array<RegType, 32> _typeIdToRegType;

	//! Word name identifiers of 8-bit, 16-bit, 32-bit, and 64-bit quantities that appear in formatted text.
	ArchTypeNameId _typeNameIdTable[4];

	//! \}

	//! \name Accessors
	//! \{
	//! Returns stack pointer register id (always GP register).
	[[nodiscard]]
	ASMJIT_INLINE_NODEBUG uint32_t spRegId() const noexcept { return _spRegId; }

	//! Returns stack frame register id (always GP register).
	[[nodiscard]]
	ASMJIT_INLINE_NODEBUG uint32_t fpRegId() const noexcept { return _fpRegId; }

	//! Returns link register id, if the architecture provides it (always GP register).
	[[nodiscard]]
	ASMJIT_INLINE_NODEBUG uint32_t linkRegId() const noexcept { return _linkRegId; }

	//! Returns program counter register id, if the architecture exposes it (always GP register).
	[[nodiscard]]
	ASMJIT_INLINE_NODEBUG uint32_t pcRegId() const noexcept { return _pcRegId; }

	//! Returns a hardware stack alignment requirement.
	//!
	//! \note This is a hardware constraint. Architectures that don't constrain it would return the lowest alignment
	//! (1), however, some architectures may constrain the alignment, for example AArch64 requires 16-byte alignment.
	[[nodiscard]]
	ASMJIT_INLINE_NODEBUG uint32_t hwStackAlignment() const noexcept { return _hwStackAlignment; }

	//! Tests whether the architecture provides link register, which is used across function calls. If the link
	//! register is not provided then a function call pushes the return address on stack (X86/X64).
	[[nodiscard]]
	ASMJIT_INLINE_NODEBUG bool hasLinkReg() const noexcept { return _linkRegId != Reg::kIdBad; }

	//! Returns minimum addressable offset on stack guaranteed for all instructions.
	[[nodiscard]]
	ASMJIT_INLINE_NODEBUG uint32_t minStackOffset() const noexcept { return _minStackOffset; }

	//! Returns maximum addressable offset on stack depending on specific instruction.
	[[nodiscard]]
	ASMJIT_INLINE_NODEBUG uint32_t maxStackOffset() const noexcept { return _maxStackOffset; }

	//! Returns ISA flags of the given register `group`.
	[[nodiscard]]
	ASMJIT_INLINE_NODEBUG InstHints instFeatureHints(RegGroup group) const noexcept { return _instHints[group]; }

	//! Tests whether the given register `group` has the given `flag` set.
	[[nodiscard]]
	ASMJIT_INLINE_NODEBUG bool hasInstHint(RegGroup group, InstHints feature) const noexcept { return Support::test(_instHints[group], feature); }

	//! Tests whether the ISA provides register swap instruction for the given register `group`.
	[[nodiscard]]
	ASMJIT_INLINE_NODEBUG bool hasInstRegSwap(RegGroup group) const noexcept { return hasInstHint(group, InstHints::kRegSwap); }

	//! Tests whether the ISA provides push/pop instructions for the given register `group`.
	[[nodiscard]]
	ASMJIT_INLINE_NODEBUG bool hasInstPushPop(RegGroup group) const noexcept { return hasInstHint(group, InstHints::kPushPop); }

	[[nodiscard]]
	ASMJIT_INLINE_NODEBUG bool hasRegType(RegType type) const noexcept
	{
		if (ASMJIT_UNLIKELY(type > RegType::kMaxValue))
		{
			type = RegType::kNone;
		}
		return Support::bitTest(_supportedRegTypes, uint32_t(type));
	}

	//! Returns a table of ISA word names that appear in formatted text. Word names are ISA dependent.
	//!
	//! The index of this table is log2 of the size:
	//!   - [0] 8-bits
	//!   - [1] 16-bits
	//!   - [2] 32-bits
	//!   - [3] 64-bits
	[[nodiscard]]
	ASMJIT_INLINE_NODEBUG const ArchTypeNameId* typeNameIdTable() const noexcept { return _typeNameIdTable; }

	//! Returns an ISA word name identifier of the given `index`, see \ref typeNameIdTable() for more details.
	[[nodiscard]]
	ASMJIT_INLINE_NODEBUG ArchTypeNameId typeNameIdByIndex(uint32_t index) const noexcept { return _typeNameIdTable[index]; }

	//! \}

	//! \name Statics
	//! \{
	//! Returns a const reference to `ArchTraits` for the given architecture `arch`.
	[[nodiscard]]
	static ASMJIT_INLINE_NODEBUG const ArchTraits& byArch(Arch arch) noexcept;

	//! \}
};

ASMJIT_VARAPI const ArchTraits _archTraits[uint32_t(Arch::kMaxValue) + 1];

//! \cond
ASMJIT_INLINE_NODEBUG const ArchTraits& ArchTraits::byArch(Arch arch) noexcept { return _archTraits[uint32_t(arch)]; }
//! \endcond

//! Architecture utilities.
namespace ArchUtils
{
	ASMJIT_API Error typeIdToRegSignature(Arch arch, TypeId typeId, TypeId* typeIdOut, OperandSignature* regSignatureOut) noexcept;
} // {ArchUtils}

//! \}

ASMJIT_END_NAMESPACE

#endif // ASMJIT_CORE_ARCHTRAITS_H_INCLUDED
