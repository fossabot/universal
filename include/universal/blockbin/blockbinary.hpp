#pragma once
// blockbinary.hpp: parameterized blocked binary number system representing a 2's complement binary number
//
// Copyright (C) 2017-2020 Stillwater Supercomputing, Inc.
//
// This file is part of the universal numbers project, which is released under an MIT Open Source license.
#include <iostream>
#include <string>
#include <sstream>

// compiler specific operators
#if defined(__clang__)
/* Clang/LLVM. ---------------------------------------------- */


#elif defined(__ICC) || defined(__INTEL_COMPILER)
/* Intel ICC/ICPC. ------------------------------------------ */


#elif defined(__GNUC__) || defined(__GNUG__)
/* GNU GCC/G++. --------------------------------------------- */


#elif defined(__HP_cc) || defined(__HP_aCC)
/* Hewlett-Packard C/aC++. ---------------------------------- */

#elif defined(__IBMC__) || defined(__IBMCPP__)
/* IBM XL C/C++. -------------------------------------------- */

#elif defined(_MSC_VER)
/* Microsoft Visual Studio. --------------------------------- */


#elif defined(__PGI)
/* Portland Group PGCC/PGCPP. ------------------------------- */

#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
/* Oracle Solaris Studio. ----------------------------------- */

#endif

namespace sw {
namespace unum {


// forward references
template<size_t nbits, typename BlockType> class blockbinary;
template<size_t nbits, typename BlockType> blockbinary<nbits, BlockType> twosComplement(const blockbinary<nbits, BlockType>&);
template<size_t nbits, typename BlockType> struct quorem;
template<size_t nbits, typename BlockType> quorem<nbits, BlockType> longdivision(const blockbinary<nbits, BlockType>&, const blockbinary<nbits, BlockType>&);

// idiv_t for blockbinary<nbits> to capture quotient and remainder during long division
template<size_t nbits, typename BlockType>
struct quorem {
	int exceptionId;
	blockbinary<nbits, BlockType> quo; // quotient
	blockbinary<nbits, BlockType> rem;  // remainder
};

// generate the 2's complement of the block binary number
template<size_t nbits, typename BlockType>
blockbinary<nbits, BlockType> twosComplement(const blockbinary<nbits, BlockType>& orig) {
	blockbinary<nbits, BlockType> twosC(orig);
	blockbinary<nbits, BlockType> plusOne(1);
	twosC.flip();
	twosC += plusOne;
	return twosC;
}

/*
NOTES

for block arithmetic, we need to manage a carry bit.
This disqualifies using uint64_t as a block type as we can't catch the overflow condition
in the same way as the other native types, uint8_t, uint16_t, uint32_t.

We could use a sint64_t and then convert to uint64_t and observe the MSB. Very different 
logic though.
*/

// a block-based 2's complement binary number
template<size_t nbits, typename BlockType = uint8_t>
class blockbinary {
public:
	static constexpr size_t bitsInByte = 8;
	static constexpr size_t bitsInBlock = sizeof(BlockType) * bitsInByte;
	static_assert(bitsInBlock <= 32, "storage unit for block arithmetic needs to be <= uint32_t");

	static constexpr size_t nrBlocks = 1 + ((nbits - 1) / bitsInBlock);
	static constexpr uint64_t storageMask = (0xFFFFFFFFFFFFFFFF >> (64 - bitsInBlock));
	static constexpr BlockType maxBlockValue = (uint64_t(1) << bitsInBlock) - 1;

	static constexpr size_t MSU = nrBlocks - 1; // MSU == Most Significant Unit
	static constexpr size_t MSU_MASK = (BlockType(0xFFFFFFFFFFFFFFFFul) >> (nrBlocks * bitsInBlock - nbits));
	static constexpr BlockType SIGN_BIT_MASK = BlockType(BlockType(1) << ((nbits - 1) % bitsInBlock));

	// constructors
	blockbinary() { setzero(); }

	blockbinary(const blockbinary&) = default;
	blockbinary(blockbinary&&) = default;

	blockbinary& operator=(const blockbinary&) = default;
	blockbinary& operator=(blockbinary&&) = default;

	/// construct a blockbinary from another
	template<size_t nnbits>
	blockbinary(const blockbinary<nnbits, BlockType>& rhs) {
		clear();
		// can simply copy the blocks in
		size_t nrBlocks = (this->nrBlocks < rhs.nrBlocks) ? this->nrBlocks : rhs.nrBlocks;
		for (size_t i = 0; i < nrBlocks; ++i) {
			_block[i] = rhs.block(i);
		}
		// enforce precondition for fast comparison by properly nulling bits that are outside of nbits
		_block[MSU] &= MSU_MASK;
	}

	// initializer for long long
	blockbinary(const long long initial_value) { *this = initial_value; }

	blockbinary& operator=(long long rhs) {
		for (unsigned i = 0; i < nrBlocks; ++i) {
			_block[i] = rhs & storageMask;
			rhs >>= bitsInBlock;
		}
		// enforce precondition for fast comparison by properly nulling bits that are outside of nbits
		_block[MSU] &= MSU_MASK;
		return *this;
	}

	// conversion operators
	explicit operator long long() const { return to_long_long(); }

	// prefix operators
	blockbinary operator-() const {
		blockbinary negated(*this);
		blockbinary plusOne(1);
		negated.flip();
		negated += plusOne;
		return negated;
	}
	// one's complement
	blockbinary operator~() const {
		blockbinary complement(*this);
		complement.flip();
		return complement;
	}
	// logic operators
	blockbinary  operator~() {
		blockbinary<nbits, BlockType> complement(*this);
		complement.flip();
		return complement;
	}
	// arithmetic operators
	blockbinary& operator+=(const blockbinary& rhs) {
		bool carry = false;
		for (unsigned i = 0; i < nrBlocks; ++i) {
			// cast up so we can test for overflow
			uint64_t l = uint64_t(_block[i]);
			uint64_t r = uint64_t(rhs._block[i]);
			uint64_t s = l + r + (carry ? uint64_t(1) : uint64_t(0));
			carry = (s > maxBlockValue ? true : false);
			_block[i] = BlockType(s);
		}
		// enforce precondition for fast comparison by properly nulling bits that are outside of nbits
		_block[MSU] &= MSU_MASK;
		return *this;
	}
	blockbinary& operator-=(const blockbinary& rhs) {
		return operator+=(twosComplement(rhs));
	}
	blockbinary& operator*=(const blockbinary& rhs) { // modulo in-place
		blockbinary base(*this);
		blockbinary multiplicant(rhs);
		clear();
		for (size_t i = 0; i < nbits; ++i) {
			if (base.at(i)) {
				operator+=(multiplicant);
			}
			multiplicant <<= 1;
		}
		// since we used operator+=, which enforces the nulling of leading bits
		// we don't need to null here
		return *this;
	}
	blockbinary& operator/=(const blockbinary& rhs) {
		quorem<nbits, BlockType> result = longdivision(*this, rhs);
		*this = result.quo;
		return *this;
	}
	blockbinary& operator%=(const blockbinary& rhs) {
		quorem<nbits, BlockType> result = longdivision(*this, rhs);
		*this = result.rem;
		return *this;
	}
	// shift left operator
	blockbinary& operator<<=(long bitsToShift) {
		if (bitsToShift < 0) return operator>>=(-bitsToShift);
		if (bitsToShift > long(nbits)) bitsToShift = nbits; // clip to max
		signed blockShift = 0;
		if (bitsToShift >= long(bitsInBlock)) {
			blockShift = bitsToShift / bitsInBlock;
			for (signed i = signed(MSU); i >= blockShift; --i) {
				_block[i] = _block[i - blockShift];
			}
			for (signed i = blockShift - 1; i >= 0; --i) {
				_block[i] = BlockType(0);
			}
			// adjust the shift
			bitsToShift -= (long)(blockShift * bitsInBlock);
			if (bitsToShift == 0) return *this;
		}
		// construct the mask for the upper bits in the block that need to move to the higher word
		BlockType mask = 0xFFFFFFFFFFFFFFFF << (bitsInBlock - bitsToShift);
		for (unsigned i = MSU; i > 0; --i) {
			_block[i] <<= bitsToShift;
			// mix in the bits from the right
			BlockType bits = (mask & _block[i - 1]);
			_block[i] |= (bits >> (bitsInBlock - bitsToShift));
		}
		_block[0] <<= bitsToShift;
		return *this;
	}
	// shift right operator
	blockbinary& operator>>=(long bitsToShift) {
		if (bitsToShift < 0) return operator<<=(-bitsToShift);
		if (bitsToShift > long(nbits)) bitsToShift = nbits; // clip to max
		size_t blockShift = 0;
		if (bitsToShift >= long(bitsInBlock)) {
			blockShift = bitsToShift / bitsInBlock;
			if (MSU > blockShift) {
				// shift by blocks
				for (size_t i = 0; i <= MSU - blockShift; ++i) {
					_block[i] = _block[i + blockShift];
				}
			}
			for (size_t i = 1 + MSU - blockShift; i <= MSU; ++i) {
				_block[i] = BlockType(0);
			}
			// adjust the shift
			bitsToShift -= (long)(blockShift * bitsInBlock);
			if (bitsToShift == 0) return *this;
		}
		BlockType mask = 0xFFFFFFFFFFFFFFFF >> (64 - bitsInBlock);
		mask >>= (bitsInBlock - bitsToShift); // this is a mask for the lower bits in the block that need to move to the lower word
		for (unsigned i = 0; i < MSU; ++i) {
			_block[i] >>= bitsToShift;
			// mix in the bits from the left
			BlockType bits = (mask & _block[i + 1]);
			_block[i] |= (bits << (bitsInBlock - bitsToShift));
		}
		_block[MSU] >>= bitsToShift;
		return *this;
	}

	// modifiers
	 // clear a block binary number
	inline void clear() {
		for (size_t i = 0; i < nrBlocks; ++i) {
			_block[i] = BlockType(0);
		}
	}
	inline void setzero() { clear(); }
	inline void reset(size_t i) {
		if (i < nbits) {
			BlockType block = _block[i / bitsInBlock];
			BlockType mask = ~(1 << (i % bitsInBlock));
			_block[i / bitsInBlock] = block & mask;
			return;
		}
		throw "blockbinary<nbits, BlockType>.reset(index): bit index out of bounds";
	}
	inline void set(size_t i, bool v = true) {
		if (i < nbits) {
			BlockType block = _block[i / bitsInBlock];
			BlockType null = ~(1 << (i % bitsInBlock));
			BlockType bit = (v ? 1 : 0);
			BlockType mask = (bit << (i % bitsInBlock));
			_block[i / bitsInBlock] = (block & null) | mask;
			return;
		}
		throw "blockbinary<nbits, BlockType>.set(index): bit index out of bounds";
	}
	void set_raw_bits(uint64_t value) {
		for (unsigned i = 0; i < nrBlocks; ++i) {
			_block[i] = value & storageMask;
			value >>= bitsInBlock;
		}
		// enforce precondition for fast comparison by properly nulling bits that are outside of nbits
		_block[MSU] &= MSU_MASK;
	}
	// in-place one's complement
	inline blockbinary& flip() {
		for (unsigned i = 0; i < nrBlocks; ++i) {
			_block[i] = ~_block[i];
		}
		// assert precondition of properly nulled leading non-bits
		_block[MSU] = _block[MSU] & MSU_MASK; 
		return *this;
	}

	// selectors
	inline bool sign() const { return _block[MSU] & SIGN_BIT_MASK; }
	inline bool ispos() const { return !sign(); }
	inline bool isneg() const { return sign(); }
	inline bool iszero() const {
		for (size_t i = 0; i < nrBlocks; ++i) if (_block[i] != 0) return false;
		return true;
	}
	inline bool isodd() const {	return _block[0] & 0x1;	}
	inline bool iseven() const { return !isodd(); }
	inline bool at(size_t i) const {
		if (i < nbits) {
			BlockType word = _block[i / bitsInBlock];
			BlockType mask = (BlockType(1) << (i % bitsInBlock));
			return (word & mask);
		}
		throw "bit index out of bounds";
	}
	inline uint8_t nibble(size_t n) const {
		if (n < (1 + ((nbits - 1) >> 2))) {
			BlockType word = _block[(n * 4) / bitsInBlock];
			int nibbleIndexInWord = n % (bitsInBlock >> 2);
			BlockType mask = 0xF << (nibbleIndexInWord*4);
			BlockType nibblebits = mask & word;
			return (nibblebits >> (nibbleIndexInWord*4));
		}
		throw "nibble index out of bounds";
	}
	inline BlockType block(size_t b) const {
		if (b < nrBlocks) {
			return _block[b];
		}
		throw "block index out of bounds";
	}
	
	// return the position of the most significant bit, -1 if v == 0
	inline signed msb() const {
		for (signed i = int(MSU); i >= 0; --i) {
			if (_block[i] != 0) {
				BlockType mask = (BlockType(1) << (bitsInBlock-1));
				for (signed j = bitsInBlock - 1; j >= 0; --j) {
					if (_block[i] & mask) {
						return i * bitsInBlock + j;
					}
					mask >>= 1;
				}
			}
		}
		return -1; // no significant bit found, all bits are zero
	}
	// conversion to native types
	long long to_long_long() const {
		constexpr unsigned sizeoflonglong = 8 * sizeof(long long);
		long long ll = 0;
		long long mask = 1;
		unsigned upper = (nbits < sizeoflonglong ? nbits : sizeoflonglong);
		for (unsigned i = 0; i < upper; ++i) {
			ll |= at(i) ? mask : 0;
			mask <<= 1;
		}
		if (sign() && upper < sizeoflonglong) { // sign extend
			for (unsigned i = upper; i < sizeoflonglong; ++i) {
				ll |= mask;
				mask <<= 1;
			}
		}
		return ll;
	}

	// determine the rounding mode: -1 round down, 0 tie, 1 round up
	int roundingMode(unsigned guardBitIndex) const {
		int rv = 0;

		return rv;
	}

protected:
	// HELPER methods

private:
	BlockType _block[nrBlocks];

	// integer - integer logic comparisons
	template<size_t N, typename B>
	friend bool operator==(const blockbinary<N, B>& lhs, const blockbinary<N, B>& rhs);
	template<size_t N, typename B>
	friend bool operator!=(const blockbinary<N, B>& lhs, const blockbinary<N, B>& rhs);
	// the other logic operators are defined in terms of arithmetic terms
};

///////////////////////////////////////////////////////////////////////////////
// logic operators

template<size_t N, typename B>
inline bool operator==(const blockbinary<N, B>& lhs, const blockbinary<N, B>& rhs) {
	for (size_t i = 0; i < lhs.nrBlocks; ++i) {
		if (lhs._block[i] != rhs._block[i]) {
			return false;
		}
	}
	return true;
}
template<size_t N, typename B>
inline bool operator!=(const blockbinary<N, B>& lhs, const blockbinary<N, B>& rhs) {
	return !operator==(lhs, rhs);
}
template<size_t N, typename B>
inline bool operator<(const blockbinary<N, B>& lhs, const blockbinary<N, B>& rhs) {
	blockbinary<N, B> diff = lhs - rhs;
	return diff.isneg();
}
template<size_t N, typename B>
inline bool operator<=(const blockbinary<N, B>& lhs, const blockbinary<N, B>& rhs) {
	blockbinary<N, B> diff = lhs - rhs;
	return diff.isneg() || diff.iszero();
}
template<size_t N, typename B>
inline bool operator>(const blockbinary<N, B>& lhs, const blockbinary<N, B>& rhs) {
	blockbinary<N, B> diff = lhs - rhs;
	return diff.ispos();
}
template<size_t N, typename B>
inline bool operator>=(const blockbinary<N, B>& lhs, const blockbinary<N, B>& rhs) {
	blockbinary<N, B> diff = lhs - rhs;
	return diff.ispos() || diff.iszero();
}
///////////////////////////////////////////////////////////////////////////////
// binary operators

template<size_t nbits, typename BlockType>
inline blockbinary<nbits, BlockType> operator+(const blockbinary<nbits, BlockType>& a, const blockbinary<nbits, BlockType>& b) {
	blockbinary<nbits, BlockType> c(a);
	return c += b;
}
template<size_t nbits, typename BlockType>
inline blockbinary<nbits, BlockType> operator-(const blockbinary<nbits, BlockType>& a, const blockbinary<nbits, BlockType>& b) {
	blockbinary<nbits, BlockType> c(a);
	return c -= b;
}
template<size_t nbits, typename BlockType>
inline blockbinary<nbits, BlockType> operator*(const blockbinary<nbits, BlockType>& a, const blockbinary<nbits, BlockType>& b) {
	blockbinary<nbits, BlockType> c(a);
	return c *= b;
}
template<size_t nbits, typename BlockType>
inline blockbinary<nbits, BlockType> operator/(const blockbinary<nbits, BlockType>& a, const blockbinary<nbits, BlockType>& b) {
	blockbinary<nbits, BlockType> c(a);
	return c /= b;
}
template<size_t nbits, typename BlockType>
inline blockbinary<nbits, BlockType> operator%(const blockbinary<nbits, BlockType>& a, const blockbinary<nbits, BlockType>& b) {
	blockbinary<nbits, BlockType> c(a);
	return c %= b;
}


// divide a by b and return both quotient and remainder
template<size_t nbits, typename BlockType>
quorem<nbits, BlockType> longdivision(const blockbinary<nbits, BlockType>& _a, const blockbinary<nbits, BlockType>& _b) {
	quorem<nbits, BlockType> result = { 0, 0, 0 };
	if (_b.iszero()) {
		result.exceptionId = 1; // division by zero
		return result;
	}
	// generate the absolute values to do long division 
	// 2's complement special case -max requires an signed int that is 1 bit bigger to represent abs()
	bool a_sign = _a.sign();
	bool b_sign = _b.sign();
	bool result_negative = (a_sign ^ b_sign);
	// normalize to positive, which requires expension by 1-bit
	blockbinary<nbits + 1, BlockType> a; a = (a_sign ? -_a : _a);
	blockbinary<nbits + 1, BlockType> b; b = (b_sign ? -_b : _b);

	if (a < b) {
		result.rem = _a; // a % b = a when a / b = 0
		return result;   // a / b = 0 when b > a
	}
	// initialize the long division
	blockbinary<nbits + 1, BlockType> accumulator = a;
	// prepare the subtractand
	blockbinary<nbits + 1, BlockType> subtractand = b;
	int msb_b = b.msb();
	int msb_a = a.msb();
	int shift = msb_a - msb_b;
	subtractand <<= shift;
	// long division
	for (int i = shift; i >= 0; --i) {
		if (subtractand <= accumulator) {
			accumulator -= subtractand;
			result.quo.set(i);
		}
		else {
			result.quo.reset(i);
		}
		subtractand >>= 1;
	}
	if (result_negative) {  // take 2's complement
		result.quo.flip();
		result.quo += 1;
	}
	if (_a.isneg()) {
		result.rem = -accumulator;
	}
	else {
		result.rem = accumulator;
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////////
// specialty binary operators

// unrounded addition, returns a blockbinary that is of size nbits+1
template<size_t nbits, typename BlockType>
inline blockbinary<nbits + 1, BlockType> uradd(const blockbinary<nbits, BlockType>& a, const blockbinary<nbits, BlockType>& b) {
	blockbinary<nbits + 1, BlockType> result(a);
	return result += blockbinary<nbits + 1, BlockType>(b);
}

// unrounded multiplication, returns a blockbinary that is of size 2*nbits
template<size_t nbits, typename BlockType>
inline blockbinary<2*nbits, BlockType> urmul(const blockbinary<nbits, BlockType>& a, const blockbinary<nbits, BlockType>& b) {
	blockbinary<2 * nbits, BlockType> result;
	blockbinary<2 * nbits, BlockType> multiplicant(b);
	for (size_t i = 0; i < nbits; ++i) {
		if (a.at(i)) {
			result += multiplicant;
		}
		multiplicant <<= 1;
	}
	// since we used operator+=, which enforces the nulling of leading bits
	// we don't need to null here
	return result;
}

//////////////////////////////////////////////////////////////////////////////
// conversions to string representations

// create a binary representation of the storage
template<size_t nbits, typename BlockType>
std::string to_binary(const blockbinary<nbits, BlockType>& number, bool nibbleMarker = false) {
	std::stringstream ss;
	ss << 'b';
	for (int i = int(nbits - 1); i >= 0; --i) {
		ss << (number.at(i) ? '1' : '0');
		if (i > 0 && (i % 4) == 0) ss << '\'';
	}
	return ss.str();
}

// local helper to display the contents of a byte array
template<size_t nbits, typename BlockType>
std::string to_hex(const blockbinary<nbits, BlockType>& number, bool wordMarker = false) {
	static constexpr size_t bitsInByte = 8;
	static constexpr size_t bitsInBlock = sizeof(BlockType) * bitsInByte;
	char hexChar[16] = {
		'0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
	};
	std::stringstream ss;
	ss << "0x" << std::hex;
	int nrNibbles = int(1 + ((nbits - 1) >> 2));
	for (int n = nrNibbles - 1; n >= 0; --n) {
		uint8_t nibble = number.nibble(n);
		ss << hexChar[nibble];
		if (n > 0 && ((n * 4) % bitsInBlock) == 0) ss << '\'';
	}
	return ss.str();
}

} // namespace unum
} // namespace sw