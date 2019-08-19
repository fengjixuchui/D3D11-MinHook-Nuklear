#pragma once

#pragma warning (push)
#pragma warning (disable: 4005)
#include <d3d9.h>
#include <d3d11.h>
#include <d3dx9.h>
#include <d3dx10.h>
#include <d3dx11.h>
#include <D3DX11tex.h>
#include <DXGI.h>
#pragma warning (pop)
#include <stdio.h>
#include <Windows.h>

#include "Helper.h"
#include "D3D11StateSaver.h"
#include "D3D11Shader.h"
#include "FW1FontWrapper/FW1FontWrapper.h"

typedef unsigned char uchar;

class Color
{
public:
	uchar A, R, G, B;

	Color()
	{
		A = R = G = B = 0;
	}

	Color(uchar A, uchar R, uchar G, uchar B)
	{
		this->A = A;
		this->R = R;
		this->G = G;
		this->B = B;
	}
};

class D3D11Renderer
{
private:
	struct COLOR_VERTEX
	{
		D3DXVECTOR3	Position;
		D3DXCOLOR Color;
	};

	IDXGISwapChain *swapChain;
	ID3D11Device *device;
	ID3D11DeviceContext *deviceContext;
	ID3D11InputLayout *mInputLayout;
	ID3D11Buffer *mVertexBuffer;
	ID3D11VertexShader *mVS;
	ID3D11PixelShader *mPS;
	ID3D11BlendState *transparency;

	IFW1Factory* pFW1Factory = NULL;
	IFW1FontWrapper* pFontWrapper = NULL;

	D3D11StateSaver *stateSaver;
	bool restoreState = false;

public:
	D3D11Renderer(IDXGISwapChain *SwapChain);
	~D3D11Renderer();

	bool Initialize();
	void DrawString(const WCHAR* pszString, float fontSize, float x, float y, UINT32 color);
	void FillRect(float x, float y, float w, float h, Color color);
	void DrawLine(float x1, float y1, float x2, float y2, Color color);
	void DrawBox(float x1, float y1, float x2, float y2, Color color);
	void DrawCircle(int x, int y, int radius, int sides, Color color);
	void DrawHealthBar(float x, float y, float w, float health, float max);
	void DrawHealthBar(float x, float y, float w, float h, float health, float max);
	float GetWidth();
	float GetHeight();
	void BeginScene();
	void EndScene();
};