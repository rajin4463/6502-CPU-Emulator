#include <stdio.h>
#include <stdlib.h>

using Byte = unsigned char;  // 8 bit
using Word = unsigned short; // 16 bit

using u32 = unsigned int;

struct Mem
{
    static constexpr u32 MAX_MEM = 1024 * 64;
    Byte Data[MAX_MEM];

    void Initialise()
    {
        for (u32 i = 0; i < MAX_MEM; i++)
        {
            Data[i] = 0;
        }
    }

    // Read 1 Byte
    Byte operator[](u32 Address) const
    {
        // Assert here Address < MAX_MEM
        return Data[Address];
    }

    // Write 1 byte to memory
    Byte &operator[](u32 Address)
    {
        // Assert here Address < MAX_MEM
        return Data[Address];
    }

    // Write 2 bytes to memory
    void Writeword( Word Value, u32 Address, u32& Cycles)
    {
        Data[Address]     = Value & 0xFF;
        Data[Address + 1] = (Value >> 8);
        Cycles -= 2;
    }
};

struct CPU
{
    Word PC; // Program Counter
    Word SP; // Stack Pointer

    Byte A, X, Y; // Registers

    // Status Flags
    Byte C : 1; // Crray Flag
    Byte Z : 1; // Zero Flag
    Byte I : 1; // Inturrept Disable
    Byte D : 1; // Decimal Mode
    Byte B : 1; // Break Command
    Byte V : 1; // Overflow Flag
    Byte N : 1; // Negavtive Flag

    void Reset(Mem &memory)
    {
        PC = 0xFFFC;
        SP = 0x00100;
        C = Z = I = D = B = V = N = 0;
        A = X = Y = 0;
        memory.Initialise();
    }

    Byte FetchByte(u32 Cycles, Mem &memory)
    {
        Byte Data = memory[PC];
        PC++;
        Cycles--;
        return Data;
    }

    Word FetchWord(u32 Cycles, Mem &memory)
    {
        // 6502 is Littel Endian
        Word Data = memory[PC];
        PC++;

        Data |= (memory[PC] << 8);
        PC++;
        Cycles -= 2;

        // If platform is not little-endian swap the bytes in word
        // Intel, AMD and Apple M chips are little-endian as of Aug 2023
        // if(PLATFORM_BIG_ENDIAN){
        //     SwapBytesInWord(Data)
        // }

        return Data;
    }

    Byte ReadByte(u32 Cycles, Byte Address, Mem &memory)
    {
        Byte Data = memory[Address];
        Cycles--;
        return Data;
    }

    // opcodes
    static constexpr Byte
        INS_LDA_IM = 0xA9,
        INS_LDA_ZP = 0xA5,
        INS_LDA_ZPX = 0xB5,
        INS_JSR = 0x20;

    void LDASetFlags()
    {
        Z = (A == 0);
        N = (A & 0b10000000) > 0;
    }

    void Execute(u32 Cycles, Mem &memory)
    {
        while (Cycles > 0)
        {
            Byte Instruction = FetchByte(Cycles, memory);
            switch (Instruction)
            {
            case INS_LDA_IM:
            {
                Byte Value = FetchByte(Cycles, memory);
                A = Value;
                LDASetFlags();
            }
            break;
            case INS_LDA_ZP:
            {
                Byte ZeroPageAddr = FetchByte(Cycles, memory);
                A = ReadByte(Cycles, ZeroPageAddr, memory);
                LDASetFlags();
            }
            break;
            case INS_LDA_ZPX:
            {
                Byte ZeroPageAddr = FetchByte(Cycles, memory);
                ZeroPageAddr += X;
                Cycles--;
                A = ReadByte(Cycles, ZeroPageAddr, memory);
                LDASetFlags();
            }
            break;
            case INS_JSR:
            {
                Word SubAddr = FetchWord(Cycles, memory);
                memory.Writeword( PC - 1, SP, Cycles);
                PC = SubAddr;
                Cycles--;
                PC = SubAddr;
                SP++;
                Cycles--;
            }
            break;
            default:
                printf("Instruction not Handeled %d", Instruction);
            }
            break;
        }
    }
};