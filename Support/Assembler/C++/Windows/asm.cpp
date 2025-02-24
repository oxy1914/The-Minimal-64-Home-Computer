// **************************************************************
//
// 'MINIMAL 64 Home Computer' Assembler (C++ Stand-alone Version)
//
//       written by Carsten Herting, last update 12.7.2023
//
// **************************************************************
// Compile with:      g++ asm.cpp -O2 -oasm.exe -s -static
//
// See end of file for licensing info and a brief description.
// **************************************************************

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <algorithm>

std::string src;														// source file
int ep=0;																		// source index

const std::vector<std::string> MNEMONICS		// Index = OpCode
{
	"NOP", "BNK", "BFF", "WIN", "INP", "INK", "OUT", "NOT", "NEG", "INC", "DEC", "CLC", "SEC",
	"LSL", "LL2", "LL3", "LL4", "LL5", "LL6", "LL7", "LSR", "ROL", "RL2", "RL3", "RL4", "RL5", "RL6", "RL7", "ROR",
  "LDI", "ADI", "SBI", "ACI", "SCI", "CPI", "ANI", "ORI", "XRI",
	"JPA", "LDA", "STA", "ADA", "SBA", "ACA", "SCA", "CPA", "ANA", "ORA", "XRA",
	"JPR", "LDR", "STR", "ADR", "SBR", "ACR", "SCR", "CPR", "ANR", "ORR",
	"CLB", "NOB", "NEB", "INB", "DEB", "ADB", "SBB", "ACB", "SCB", "ANB", "ORB", "LLB", "LRB", "RLB", "RRB",
	"CLW", "NOW", "NEW", "INW", "DEW", "ADW", "SBW", "ACW", "SCW", "LLW", "RLW",
	"JPS", "RTS", "PHS", "PLS", "LDS", "STS", "BNE", "BEQ", "BCC", "BCS", "BPL", "BMI", "BGT", "BLE",
	"TAX", "TXA", "TXY", "LXI", "LXA", "LAX", "INX", "DEX", "ADX", "SBX", "CPX", "ANX", "ORX", "XRX",
	"TAY", "TYA", "TYX", "LYI", "LYA", "LAY", "INY", "DEY", "ADY", "SBY", "CPY", "ANY", "ORY", "XRY", "HLT"
};

char opCode()
{
	std::string str = src.substr(ep, 3);
	for(int i=0; i<MNEMONICS.size(); i++) if (str == MNEMONICS[i]) return i;
	return -1;
}

std::string linenr()
{
	size_t n=1;
	for (size_t i=0; i<=ep; i++) if (src[i] == '\n') n++;
	return std::to_string(n);
}

char findelem()										// find the next element starting from whitespace
{																	// return 0 for EOF element
	while (true)
	{
		if(src[ep] != 0 && (src[ep] <= 32 || src[ep] == ',')) ep++;			// move over whitespace and comma but not over EOF
		else
		{
			if (src[ep] == ';')					// enter ; comment
			{
				while (true)
				{
					ep++;
					if (src[ep] == '\n') { ep++; break; }	// consume LF and end comment loop
					if (src[ep] == 0) return 0;						// next element EOF found
				}
			}
			else return src[ep];											// next element found
		}
	}	
}

int elength()											// calculate the length of the element
{																	// should only be called at the start of a valid element != EOF
	if (src[ep] == '\'')						// enter special case '...' string?
	{
		int n = ep + 1;								// start looking beyond string start marker
		while (src[n] != '\'' && src[n] != '\n')
		{
			if (src[n] == 0) return n - ep;
			n++;		// expect string end marker or LF or EOF
		}
		return n + 1 - ep;						// return length including start and end markers
	}
	else														// case usual element
	{
		int n = ep + 1;								// look beyond elements first char
		while (src[n] > 32 && src[n] != ',' && src[n] != ';') n++;		// consume printable characters except , and ;
		return n - ep;
	}
}

class HexPrinter
{
	public:
		HexPrinter() {}
		~HexPrinter() { if (used > 0) emitBuffer(); std::cout << ":00000001FF\n"; }
		void SetAddress(int addr) { if (used > 0) emitBuffer(); address = addr; }
		void Emit(uint8_t b) { buffer[used++] = b; if (used == 16) emitBuffer(); }
	protected:
		void emitBuffer()
		{
			std::cout << ":" << std::hex << std::uppercase << std::setfill('0');
			uint8_t pch = (address & 0xff00)>>8;
			uint8_t pcl = address & 0x00ff;
			std::cout << std::setw(2) << used << std::setw(2) << int(pch) << std::setw(2) << int(pcl) << "00";
			uint8_t checksum = used + pch + pcl;
			for(int i=0; i<used; i++) { std::cout << std::setw(2) << int(buffer[i]); checksum += buffer[i]; }
			std::cout << std::setw(2) << int((~checksum + 1) & 0xff) << "\n";
			address += used; used = 0;
		}
		uint8_t buffer[16]{};
		int used{ 0 };
		int address{ -1 };
};

int main(int argc, char *argv[])
{
	bool dosym = false;																// by default don't output a symbol table
	std::string symtag = "";													// by default don't use any symbol tag
	int filenamepos = 0;															// extract possible -s parameter and filename
	for (int i=1; i<argc; i++)												// index zero contains "asm" itself
	{
		if (argv[i][0] == '-' && argv[i][1] == 's')	{ dosym = true; symtag = std::string(&argv[i][2]); }
		else filenamepos = i;														// nope, plain filename => remember it's index inside argv[]
	}

	if (filenamepos > 0)															// does a valid argument position of a filename exist?
	{
		std::ifstream f(argv[filenamepos]);
		if (f.is_open())
		{
			std::cout << ("Assembling \"" + std::string(argv[filenamepos]) + "\"...\n\n");
			std::getline(f, src, '\0'); f.close(); 										// read in the source text

			// PASS 1: Calculates addresses of label definition
			std::vector<std::string> labels;        							  	// Liste aller Label-Definitionen mit ":"
			std::vector<int> labelpc;     														// Adresse aller Label-Definitionen
			int	pc = 0;																								// program counter
			ep = 0;
			while (findelem() != 0)
			{
				int elen = elength();
				if (src[ep+elen-1] == ':')															// label definitions?
				{
					std::string def = src.substr(ep, elen-1);
					bool isknown = false;																	// is it a known label?
					for(int i=0; i<labels.size(); i++)										// ersetzte label referenece durch value
						if (def == labels[i]) { std::cout << "ERROR in line " << linenr() << ": \'" << def << "\' already exists.\n"; exit(1); }
					labels.emplace_back(src.substr(ep, elen-1)); labelpc.emplace_back(pc);
				}
				else if (src[ep] == '#')																// preprocessor command?
				{
					if (elen == 4 && src[ep+1] == 'o' && src[ep+2] == 'r' && src[ep+3] == 'g')
					{
						ep += elen; findelem(); elen = elength();						// parse next element
						if (src[ep] == '0' && src[ep+1] == 'x')
						{
							pc = std::stoi(src.substr(ep+2, 4), nullptr, 16);	// always process #org
						}
						else { std::cout << "ERROR in line " << linenr() << ": Expecting hex address after #org.\n"; exit(1); }
					}
				}
				else if (src[ep] == '\'') pc += elen-2;									// string?
				else if (src[ep] == '<' || src[ep] == '>') pc++;				// LSB and MSB operators
				else																										// SIMPLE EXPRESSSION
				{
					if (src[ep] == '+' || src[ep] == '-') { elen--; ep++; }	// consume optional leading + or -
					if (src[ep] == '0' && src[ep+1] == 'x') { if(elen > 4) pc += 2; else pc++; }		// hex word or byte
					else if (src[ep] >= '0' && src[ep] <= '9') pc++;			// plain number (8-bit only) 
					else if (elen == 3 && opCode() != -1) pc++;						// mnemonics
					else pc += 2;																					// * and labels
				}
				ep += elen;																							// hop over the processed element
			}

			if (dosym) // Ausgabe der Liste aller symbolic constants and their address values [starting with 'tag'].
			{
				for(int k=0; k<labels.size(); k++)            					// Prüfe: Ist das Element ein label?
				{
					int adr = labelpc[k];
					if (symtag == labels[k].substr(0, symtag.length()))
					{	
						std::cout << "#org 0x" << std::hex << std::setfill('0') << std::setw(2) << int((adr&0xff00)>>8) << std::setw(2) << int(adr&0x00ff) << " " << labels[k] << ":\n";
					}
				}
			}
			else
			{
				// PASS 2: Write code to memory
				pc = 0; ep = 0;																	// go back to start of source, reset program counter
				bool isemit = true;															// default: code is emitted
				HexPrinter HEX;
				while (findelem() != 0)
				{
					int elen = elength();

					if (src[ep] == '#')																				// preprocessor commands
					{
						if (elen == 5 && src[ep+1] == 'm' && src[ep+2] == 'u' && src[ep+3] == 't' && src[ep+4] == 'e') isemit = false;
						else if (elen == 5 && src[ep+1] == 'e' && src[ep+2] == 'm' && src[ep+3] == 'i' && src[ep+4] == 't') isemit = true;
						else if (elen == 4 && src[ep+1] == 'o' && src[ep+2] == 'r' && src[ep+3] == 'g')
						{
							ep+= elen; findelem(); elen = elength();							// this #org 0x.... is already known to be parsable
							if (isemit) { pc = std::stoi(src.substr(ep+2, 4).c_str(), nullptr, 16); HEX.SetAddress(pc); }	// only accep #org while emitting
						}
						else { std::cout << "ERROR in line " << linenr() << ": Unknown preprocessor command \'" << src.substr(ep, elen) << "\'\n"; exit(1); }
					}
					else if (isemit)																					// do this only if emission is switched on
					{
						if (src[ep+elen-1] == ':');															// skip label definitions
						else if (src[ep] == '\'')																// string
						{
							for (int i=ep+1; i<ep+1+elen-2; i++) { HEX.Emit(src[i]); pc++; }
						}
						else if (elen == 3 && opCode() != -1) { HEX.Emit(opCode()); pc++; }	// mnemonic
						else																										// EXPRESSION PARSING
						{
							char lsbmsb = 0;																			// info about '<', '>', 'w' or 0
							int term = 0, expr = 0, sign = 1, x = ep;
							
							if (src[x] == '<' || src[x] == '>') lsbmsb = src[x++];// read LSB and MSB operators
							
							while (x < ep+elen)																		// parse +/- separated terms of expression
							{
								if (src[x] == '+') { sign = 1; term=0; x++; }				// take leading sign
								else if (src[x] == '-')  { sign = -1; term=0; x++; }

								if (src[x] == '0' && src[x+1] == 'x')								// hex word or byte
								{
									size_t k = src.find_first_not_of("0123456789abcdef", x+2);
									term = std::stoi(src.substr(x+2, k-(x+2)), nullptr, 16);
									if (k-x > 4 && lsbmsb == 0) lsbmsb = 'w';
									x = k;
								}
								else if (src[x] == '*') { if (lsbmsb == 0) lsbmsb = 'w'; term = pc; x++; }	// location pointer
								else if (src[x] >= '0' && src[x] <= '9')						// decimal number
								{
									while (src[x] >= '0' && src[x] <= '9') { term *= 10; term += src[x++]-'0'; }
								}
								else																								// must be a label
								{
									if (lsbmsb == 0) lsbmsb = 'w';
									size_t k = src.find_first_of(" +-\n\r\t,;\0", x);	// finde label-Ende
									std::string ref = src.substr(x, k - x);						// cut out this reference
									x = k;																						// advance over
									bool isknown = false;															// is it a known label?
									for(int i=0; i<labels.size(); i++)								// ersetzte label referenece durch value
										if (ref == labels[i]) { term = labelpc[i]; isknown = true; break; }
									if (!isknown) { std::cout << "ERROR in line " << linenr() << ": Unknown label reference \'" << ref << "\'\n"; exit(1); }
								}
								expr += sign * term;																// add/subtract this term to the expression
							}
							
							if (lsbmsb == 'w')																		// emit expression
							{
								HEX.Emit(expr & 0x00ff); pc++;											// emit LSB MSB
								HEX.Emit((expr & 0xff00)>>8); pc++;
							}
							else if (lsbmsb == '>') { HEX.Emit((expr & 0xff00)>>8); pc++; }		// MSB only
							else { HEX.Emit(expr & 0x00ff); pc++; }														// LSB only
						}
					}
					ep += elen;																								// hop over processed element // getchar();
				}
			}
		}
		else std::cout << ("Can't open \"" + std::string(argv[filenamepos]) + "\".\n");
	}
	else
	{
		std::cout << "MINIMAL 64 Home Computer Assembler\n";		// output help screen
		std::cout << "by Carsten Herting (slu4) 2023\n\n";
		std::cout << "  Usage: asm <sourcefile> [-s[<tag>]]\n\n";
		std::cout << "assembles a <sourcefile> to machine code and outputs\n";
		std::cout << "the result in Intel HEX format to the console.\n\n";
		std::cout << "  -s[<tag>]  outputs a list of symbolic constants\n";
		std::cout << "             [starting with <tag>] and their values.\n";
	}

	return 0;
}

// FUNCTIONAL DESCRIPTION

// This assembler reads an assembly language source file, translates the instructions into machine code,
// and emits the code in Intel Hex format. The program is structured into two main passes: Pass 1: Parse
// the source code and store label definitions and their program counter (PC) values. Pass 2: Emit the
// machine code based on the parsed instructions and label values. Here's an overview of the key components
// and how they work together:
//   1) File handling: The program reads the input assembly file specified as a command-line argument 
// and stores the content in a string called src.
//   2) findelem() function: This function iterates over the source code, counting lines and skipping 
// whitespaces and comments. It returns the length of the current element in the source code. It is 
// used in both Pass 1 and Pass 2.
//   3) Pass 1: The program iterates through the source code using findelem() to identify label definitions,
// mnemonics, preprocessor commands, and expressions. It stores the label definitions and their corresponding
// PC values in a dictionary called labels. It also increments the PC based on the type of element encountered.
//   4) Pass 2: The program resets the PC and iterates through the source code again, emitting machine code 
// based on the parsed instructions and label values. During this pass, it handles preprocessor commands
// (e.g., #org, #mute, and #emit), label definitions, mnemonics, strings, operators, and expressions. The
// generated machine code is stored in an IntelHex object called 'ih'.
//   5) Output: The program writes the output to the console in Intel Hex format.

// EBNF of Minimal 64 Assembly Language Definition by Carsten Herting (slu4) 2023

// char       = ? any character ?
// letter     = 'a' | ... | 'z' | 'A' | ... | 'Z'
// digit      = '0' | ... | '9'
// hexdigit   = digit | 'a' | ... | 'f'           (* only lower-case allowed *)
// hexnum     = '0x', hexdigit, {hexdigit}
// dec_lsb    = digit, {digit}                    (* only the lsb part of dec numbers will be used *)
// label      = ('_' | letter), {'_' | letter | digit}
// gap        = ' ' | ',' | '\t' | '\r' | '\n'
// add-op     = '+' | '-'
// lsbmsb-op  = '<' | '>'
// mnemonic   = 'NOP' | ... | 'HLT'
// pre-proc   = '#emit' | '#mute' | ( '#org', gap, {gap}, hexnum )
// comment    = ';', { char - ( '\n' | '\0' ) }
// definition = label, ':'
// string     = "'", { char - ( '\n' | '\0' | "'" ) }, "'"
// program    = {gap}, [ element, { gap, {gap}, element } ], {gap}, '\0'
// element    = pre-proc | comment | string | mnemonic | definition | expression
// expression = [ lsbmsb-op ], [ add-op ], term, { add-op, term }			(* 1st term determines size *)
// term       = hexnum | dec_lsb | label | '*'

// LICENSING INFORMATION

// This file is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either
// version 3 of the License, or (at your option) any later version.
// This file is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
// License for more details. You should have received a copy of the GNU General Public License along
// with this program. If not, see https://www.gnu.org/licenses/.
