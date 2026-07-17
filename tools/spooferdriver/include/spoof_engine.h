#pragma once
char SpoofCharacter(char c, const UINT8* seed, ULONG position);

BOOLEAN SpoofSerialString(PCHAR serial, SIZE_T maxLen, const UINT8* seed, const char* source);

void SpoofNvmeSerial(PCHAR serial, const UINT8* seed, const char* source);

void SpoofBinaryData(PUCHAR data, SIZE_T len, const UINT8* seed, const char* source);

BOOLEAN IsDefaultString(const char* str, SIZE_T len);
