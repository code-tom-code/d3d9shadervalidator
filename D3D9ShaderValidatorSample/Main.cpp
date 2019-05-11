#pragma once

#ifdef _DEBUG
	#define D3D_DEBUG_INFO 1
#else
	#undef D3D_DEBUG_INFO
#endif

#undef UNICODE
#undef _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdio.h> // for printf

#include <d3d9.h> // for D3D9
#include "../d3d9_shadervalidator.h" // for the Shader Validator declarations and enum definition

static void CALLBACK ShaderValidatorCallback(LPCSTR unknownString /*Seems to always be NULL?*/, 
	UINT unknownUINT /*Seems to always be 0xFFFFFFFF?*/, 
	DWORD messageCategoryFlags /*Some combination of the D3DSVERR_CATEGORY_FLAGS_* flags*/, 
	D3DSVERROR_ID errorID /*Message ID*/, 
	LPCSTR messageString /*Helpful text containing a message description*/, 
	LPVOID lParam /*Same user-specified lParam from IDirect3DShaderValidator9::Begin()*/)
{
	// This format string should match the "error X5...: %s" format seen in the official D3D9 error messages that get spit out of the runtime.
	printf("%s X5%u: %s\n", 
		(messageCategoryFlags & D3DSVERR_CATEGORY_FLAGS_WARNING) ? "warning" : "error", 
		errorID, 
		messageString);
}

// Make sure to call free() on the memory after you're done with it
DWORD* const LoadShaderToMemory(const char* const filename)
{
	if (!filename)
	{
		printf("Error: NULL filename passed to LoadShaderToMemory!\n");
		return NULL;
	}

	if (!*filename)
	{
		printf("Error: Empty filename passed to LoadShaderToMemory!\n");
		return NULL;
	}

	char buffer[MAX_PATH] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
	sprintf(buffer, "%s.cso", filename);
#pragma warning(pop)
	HANDLE hFile = CreateFileA(buffer, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
#pragma warning(push)
#pragma warning(disable:4996)
#ifdef _M_X64
#ifdef _DEBUG
		sprintf(buffer, "..\\x64\\Debug\\%s.cso", filename);
#else
		sprintf(buffer, "..\\x64\\Release\\%s.cso", filename);
#endif
#elif defined(_M_X86)
#ifdef _DEBUG
		sprintf(buffer, "..\\Debug\\%s.cso", filename);
#else
		sprintf(buffer, "..\\Release\\%s.cso", filename);
#endif
#else
#error Unknown architecture (ARM perhaps?)
#endif // #ifdef _M_X64_
#pragma warning(pop)
		hFile = CreateFileA(buffer, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			printf("Error: Cannot open file \"%s\" for reading!\n", filename);
			return NULL;
		}
	}

	const DWORD fileSize = GetFileSize(hFile, NULL);
	if (!fileSize)
	{
		CloseHandle(hFile);
		printf("Error getting file size\n");
		return NULL;
	}

	DWORD* const data = (DWORD* const)malloc(fileSize);
	if (!data)
	{
		CloseHandle(hFile);
		printf("Error mallocing %u bytes!\n", fileSize);
		return NULL;
	}

	DWORD bytesRead = 0;
	if (!ReadFile(hFile, data, fileSize, &bytesRead, NULL) || bytesRead != fileSize)
	{
		free(data);
		CloseHandle(hFile);
		printf("Error in ReadFile!\n");
		return NULL;
	}
	CloseHandle(hFile);

	return data;
}

static int ValidateShader(const DWORD* const shaderBytecode)
{
	HMODULE hmd3d9 = LoadLibraryA("d3d9.dll");
	if (!hmd3d9)
	{
		printf("Error in LoadLibrary()\n");
		return 1;
	}

	const Direct3DShaderValidatorCreate9Type Direct3DShaderValidatorCreate9 = (const Direct3DShaderValidatorCreate9Type)GetProcAddress(hmd3d9, "Direct3DShaderValidatorCreate9");
	if (!Direct3DShaderValidatorCreate9)
	{
		printf("Error in GetProcAddress()\n");
		FreeLibrary(hmd3d9);
		return 1;
	}

	IDirect3DShaderValidator9* ShaderValidator = (*Direct3DShaderValidatorCreate9)();
	if (!ShaderValidator)
	{
		printf("Shader validator create failed!\n");
		FreeLibrary(hmd3d9);
		return 1;
	}

	HRESULT beginHR = ShaderValidator->Begin(&ShaderValidatorCallback, NULL, 0);
	if (FAILED(beginHR) )
	{
		printf("Shader validator begin failed with HR: 0x%08X\n", beginHR);
		ShaderValidator->Release();
		ShaderValidator = NULL;
		FreeLibrary(hmd3d9);
		return 1;
	}

	const DWORD* currentInstruction = shaderBytecode;
	HRESULT instructionHR = S_OK;

	// All D3D8 and D3D9 shaders must start with a version token first.
	// Source for the version token here: https://docs.microsoft.com/en-us/windows-hardware/drivers/display/version-token
	HRESULT versionHR = ShaderValidator->Instruction("Version Token", 0, currentInstruction, 1);
	++currentInstruction;

	// Primary shader parsing loop (for shader model 2 and 3 shaders):
	for (;;)
	{
		switch (*currentInstruction & D3DSI_OPCODE_MASK)
		{
		default: // Most of all of the shader opcodes go here
		{
			// Add one because the instruction token itself is not counted in the instruction length.
			// Note that this method *will not work* for Shader Model 1.x shaders. It wasn't until Shader Model 2.0 that this "instruction length" field was
			// added to the bytecode. This means that for vs_1_1 and for ps_1_1 thru ps_1_4 shaders, you'll need to have a table somewhere of instruction-lengths
			// and use that for determining the "numTokensForInstruction" value instead.
			// Source: https://docs.microsoft.com/en-us/windows-hardware/drivers/display/instruction-token
			const DWORD numTokensForInstruction = 1 + ( (D3DSI_INSTLENGTH_MASK & *currentInstruction) >> D3DSI_INSTLENGTH_SHIFT);
			instructionHR = ShaderValidator->Instruction("Instruction", 0, currentInstruction, numTokensForInstruction);
			if (FAILED(instructionHR) )
				goto instructionError;
			currentInstruction += numTokensForInstruction;
		}
			break;
		case D3DSIO_PHASE:
			instructionHR = ShaderValidator->Instruction("Phase", 0, currentInstruction, 1);
			if (FAILED(instructionHR) )
				goto instructionError;
			++currentInstruction;
			break;
		case D3DSIO_COMMENT:
		{
			// Add one because the comment token itself is not counted in the comment size.
			// Source: https://docs.microsoft.com/en-us/windows-hardware/drivers/display/comment-token
			const DWORD numTokensForComment = 1 + ( (D3DSI_COMMENTSIZE_MASK & *currentInstruction) >> D3DSI_COMMENTSIZE_SHIFT);
			instructionHR = ShaderValidator->Instruction("Comment", 0, currentInstruction, numTokensForComment);
			if (FAILED(instructionHR) )
				goto instructionError;
			currentInstruction += numTokensForComment;
		}
			break;
		case D3DSIO_END:
		{
			// Source for the END token here: https://docs.microsoft.com/en-us/windows-hardware/drivers/display/end-token
			instructionHR = ShaderValidator->Instruction("End", 0, currentInstruction, 1);
			if (FAILED(instructionHR) )
				goto instructionError;

			HRESULT endHR = ShaderValidator->End();
			ShaderValidator->Release();
			ShaderValidator = NULL;
			FreeLibrary(hmd3d9);
			if (FAILED(endHR) )
			{
				printf("Shader validator end failed with HR: 0x%08X\n", endHR);
				return 1;
			}

			printf("Shader validation completed successfully without errors!\n");
			return 0;
		}
		}

		if (FAILED(instructionHR) )
			goto instructionError;
	}

instructionError:
	printf("Shader validator instruction failed with HR: 0x%08X\n", instructionHR);
	ShaderValidator->Release();
	ShaderValidator = NULL;
	FreeLibrary(hmd3d9);
	return 1;
}

int main(const unsigned argc, const char* const argv[])
{
	DWORD* loadedPS = LoadShaderToMemory("TestPS");
	DWORD* loadedVS = LoadShaderToMemory("TestVS");

	if (ValidateShader(loadedPS) != 0) // This should succeed
		return 1;

	if (ValidateShader(loadedVS) != 0) // This should succeed
		return 1;

	// Let's mess some stuff up for testing purposes! Let's mark our vertex shader as if it were a pixel shader:
	*loadedVS |= D3DPS_VERSION(0, 0);

	if (ValidateShader(loadedVS) != 0) // This should fail
		return 1;

	free(loadedPS);
	loadedPS = NULL;
	free(loadedVS);
	loadedVS = NULL;

	return 0;
}
