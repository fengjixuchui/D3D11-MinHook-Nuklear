#include "D3D11Renderer.h"

D3D11Renderer::D3D11Renderer(IDXGISwapChain* SwapChain)
{
	this->device = NULL;
	this->deviceContext = NULL;
	this->mVS = NULL;
	this->mPS = NULL;
	this->transparency = NULL;
	this->mInputLayout = NULL;
	this->mVertexBuffer = NULL;

	this->swapChain = SwapChain;

	this->stateSaver = new D3D11StateSaver();
}

D3D11Renderer::~D3D11Renderer()
{
	SAFE_DELETE(this->stateSaver);
	SAFE_RELEASE(this->mVS);
	SAFE_RELEASE(this->mPS);
	SAFE_RELEASE(this->transparency);
	SAFE_RELEASE(this->mInputLayout);
	SAFE_RELEASE(this->mVertexBuffer);
	SAFE_RELEASE(this->swapChain);
	SAFE_RELEASE(this->device);
	SAFE_RELEASE(this->deviceContext);
}

bool D3D11Renderer::Initialize()
{
	HRESULT hr;

	if (!this->swapChain)
		return false;

	this->swapChain->GetDevice(__uuidof(this->device), (void**)& this->device);
	if (!this->device)
		return false;

	this->device->GetImmediateContext(&this->deviceContext);
	if (!this->deviceContext)
		return false;

	typedef HRESULT(__stdcall * D3DCompile_t)(LPCVOID pSrcData, SIZE_T SrcDataSize, LPCSTR pSourceName, const D3D_SHADER_MACRO * pDefines, ID3DInclude * pInclude, LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1, UINT Flags2, ID3DBlob * *ppCode, ID3DBlob * ppErrorMsgs);
	D3DCompile_t myD3DCompile = (D3DCompile_t)GetProcAddress(GetD3DCompiler(), "D3DCompile");
	if (!myD3DCompile)
		return false;

	ID3D10Blob* VS, * PS;
	hr = myD3DCompile(D3D11FillShader, sizeof(D3D11FillShader), NULL, NULL, NULL, "VS", "vs_4_0", 0, 0, &VS, NULL);
	if (FAILED(hr))
		return false;

	hr = this->device->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &this->mVS);
	if (FAILED(hr))
	{
		SAFE_RELEASE(VS);
		return false;
	}

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	hr = this->device->CreateInputLayout(layout, ARRAYSIZE(layout), VS->GetBufferPointer(), VS->GetBufferSize(), &this->mInputLayout);
	SAFE_RELEASE(VS);
	if (FAILED(hr))
		return false;

	myD3DCompile(D3D11FillShader, sizeof(D3D11FillShader), NULL, NULL, NULL, "PS", "ps_4_0", 0, 0, &PS, NULL);
	if (FAILED(hr))
		return false;

	hr = this->device->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &this->mPS);
	if (FAILED(hr))
	{
		SAFE_RELEASE(PS);
		return false;
	}

	D3D11_BUFFER_DESC bufferDesc;

	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = 4 * sizeof(COLOR_VERTEX);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;

	hr = this->device->CreateBuffer(&bufferDesc, NULL, &this->mVertexBuffer);
	if (FAILED(hr))
		return false;

	D3D11_BLEND_DESC blendStateDescription;
	ZeroMemory(&blendStateDescription, sizeof(blendStateDescription));

	blendStateDescription.RenderTarget[0].BlendEnable = TRUE;
	blendStateDescription.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendStateDescription.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDescription.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDescription.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendStateDescription.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDescription.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDescription.RenderTarget[0].RenderTargetWriteMask = 0x0f;

	hr = this->device->CreateBlendState(&blendStateDescription, &this->transparency);
	if (FAILED(hr))
		return false;

	FW1CreateFactory(FW1_VERSION, &pFW1Factory);
	pFW1Factory->CreateFontWrapper(this->device, L"Roboto", &this->pFontWrapper);

	pFW1Factory->Release();

	return true;
}

void D3D11Renderer::DrawString(const WCHAR* string, float fontSize, float x, float y, UINT32 color) {
	// UINT Color format:
	// | 0x | 00    | 00    | FF    | 00    |
	// | 0x | Alpha | Red   | Green | Blue  |
	// | 0x | 0-255 | 0-255 | 0-255 | 0-255 |
	// Image: http://www.directxtutorial.com/Lessons/9/B-D3DGettingStarted/3/16.gif

	// Example (Blue):
	// 0x ff ff 16 12 / 0xffff1612
	this->pFontWrapper->DrawString(this->deviceContext, string, fontSize, x, y, color, FW1_RESTORESTATE);
}

void D3D11Renderer::FillRect(float x, float y, float w, float h, Color color)
{
	if (this->deviceContext == NULL)
		return;

	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	this->deviceContext->OMSetBlendState(this->transparency, blendFactor, 0xffffffff);

	int a = color.A & 0xff;
	int r = color.R & 0xff;
	int g = color.G & 0xff;
	int b = color.B & 0xff;

	UINT viewportNumber = 1;

	D3D11_VIEWPORT vp;

	this->deviceContext->RSGetViewports(&viewportNumber, &vp);

	float x0 = x;
	float y0 = y;
	float x1 = x + w;
	float y1 = y + h;

	float xx0 = 2.0f * (x0 - 0.5f) / vp.Width - 1.0f;
	float yy0 = 1.0f - 2.0f * (y0 - 0.5f) / vp.Height;
	float xx1 = 2.0f * (x1 - 0.5f) / vp.Width - 1.0f;
	float yy1 = 1.0f - 2.0f * (y1 - 0.5f) / vp.Height;

	COLOR_VERTEX* v = NULL;
	D3D11_MAPPED_SUBRESOURCE mapData;

	if (FAILED(this->deviceContext->Map(this->mVertexBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &mapData)))
		return;

	v = (COLOR_VERTEX*)mapData.pData;

	v[0].Position.x = (float)x0;
	v[0].Position.y = (float)y0;
	v[0].Position.z = 0;
	v[0].Color = D3DXCOLOR(
		((float)r / 255.0f),
		((float)g / 255.0f),
		((float)b / 255.0f),
		((float)a / 255.0f));

	v[1].Position.x = (float)x1;
	v[1].Position.y = (float)y1;
	v[1].Position.z = 0;
	v[1].Color = D3DXCOLOR(
		((float)r / 255.0f),
		((float)g / 255.0f),
		((float)b / 255.0f),
		((float)a / 255.0f));

	v[0].Position.x = xx0;
	v[0].Position.y = yy0;
	v[0].Position.z = 0;
	v[0].Color = D3DXCOLOR(
		((float)r / 255.0f),
		((float)g / 255.0f),
		((float)b / 255.0f),
		((float)a / 255.0f));

	v[1].Position.x = xx1;
	v[1].Position.y = yy0;
	v[1].Position.z = 0;
	v[1].Color = D3DXCOLOR(
		((float)r / 255.0f),
		((float)g / 255.0f),
		((float)b / 255.0f),
		((float)a / 255.0f));

	v[2].Position.x = xx0;
	v[2].Position.y = yy1;
	v[2].Position.z = 0;
	v[2].Color = D3DXCOLOR(
		((float)r / 255.0f),
		((float)g / 255.0f),
		((float)b / 255.0f),
		((float)a / 255.0f));

	v[3].Position.x = xx1;
	v[3].Position.y = yy1;
	v[3].Position.z = 0;
	v[3].Color = D3DXCOLOR(
		((float)r / 255.0f),
		((float)g / 255.0f),
		((float)b / 255.0f),
		((float)a / 255.0f));


	this->deviceContext->Unmap(this->mVertexBuffer, NULL);

	UINT Stride = sizeof(COLOR_VERTEX);
	UINT Offset = 0;

	this->deviceContext->IASetVertexBuffers(0, 1, &this->mVertexBuffer, &Stride, &Offset);
	this->deviceContext->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	this->deviceContext->IASetInputLayout(this->mInputLayout);

	this->deviceContext->VSSetShader(this->mVS, 0, 0);
	this->deviceContext->PSSetShader(this->mPS, 0, 0);
	this->deviceContext->GSSetShader(NULL, 0, 0);
	this->deviceContext->Draw(4, 0);
}

void D3D11Renderer::DrawLine(float x1, float y1, float x2, float y2, Color color)
{
	if (this->deviceContext == NULL)
		return;

	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	this->deviceContext->OMSetBlendState(this->transparency, blendFactor, 0xffffffff);

	int a = color.A & 0xff;
	int r = color.R & 0xff;
	int g = color.G & 0xff;
	int b = color.B & 0xff;

	UINT viewportNumber = 1;

	D3D11_VIEWPORT vp;

	this->deviceContext->RSGetViewports(&viewportNumber, &vp);

	float xx0 = 2.0f * (x1 - 0.5f) / vp.Width - 1.0f;
	float yy0 = 1.0f - 2.0f * (y1 - 0.5f) / vp.Height;
	float xx1 = 2.0f * (x2 - 0.5f) / vp.Width - 1.0f;
	float yy1 = 1.0f - 2.0f * (y2 - 0.5f) / vp.Height;

	COLOR_VERTEX* v = NULL;

	D3D11_MAPPED_SUBRESOURCE mapData;

	if (FAILED(this->deviceContext->Map(this->mVertexBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &mapData)))
		return;

	v = (COLOR_VERTEX*)mapData.pData;

	v[0].Position.x = xx0;
	v[0].Position.y = yy0;

	v[0].Position.z = 0;
	v[0].Color = D3DXCOLOR(
		((float)r / 255.0f),
		((float)g / 255.0f),
		((float)b / 255.0f),
		((float)a / 255.0f));

	v[1].Position.x = xx1;
	v[1].Position.y = yy1;
	v[1].Position.z = 0;
	v[1].Color = D3DXCOLOR(
		((float)r / 255.0f),
		((float)g / 255.0f),
		((float)b / 255.0f),
		((float)a / 255.0f));

	this->deviceContext->Unmap(this->mVertexBuffer, NULL);

	UINT Stride = sizeof(COLOR_VERTEX);
	UINT Offset = 0;

	this->deviceContext->IASetVertexBuffers(0, 1, &this->mVertexBuffer, &Stride, &Offset);
	this->deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
	this->deviceContext->IASetInputLayout(this->mInputLayout);

	this->deviceContext->VSSetShader(this->mVS, 0, 0);
	this->deviceContext->PSSetShader(this->mPS, 0, 0);
	this->deviceContext->GSSetShader(NULL, 0, 0);
	this->deviceContext->Draw(2, 0);
}

void D3D11Renderer::DrawBox(float x1, float y1, float x2, float y2, Color color)
{
	float width = (y2 + 15 - y1) / 4.5f;

	DrawLine(x1 - width, y1, x1 + width, y1, color);
	DrawLine(x1 - width, y2, x1 - width, y1, color);

	DrawLine(x1 + width, y2, x1 + width, y1, color);
	DrawLine(x1 - width, y2, x1 + width, y2, color);
}

//void D3D11Renderer::Draw3DBox(DWORD_PTR pActor)
//{
//	Vector3 bounds = mem->RPM<Vector3>(mem->RPM<DWORD_PTR>(pActor + 0x180, 0x8) + 0x180, 0xC);
//	Vector3 pos = GetActorPos(pActor);
//	Vector3 top1 = WorldToScreen(Vector3(pos.x + bounds.x, pos.y + bounds.y, pos.z + bounds.z), global::cameracache);
//	Vector3 top2 = WorldToScreen(Vector3(pos.x - bounds.x, pos.y + bounds.y, pos.z + bounds.z), global::cameracache);
//	Vector3 top4 = WorldToScreen(Vector3(pos.x + bounds.x, pos.y - bounds.y, pos.z + bounds.z), global::cameracache);
//	Vector3 top3 = WorldToScreen(Vector3(pos.x - bounds.x, pos.y - bounds.y, pos.z + bounds.z), global::cameracache);
//	Vector3 bot1 = WorldToScreen(Vector3(pos.x + bounds.x, pos.y + bounds.y, pos.z - bounds.z), global::cameracache);
//	Vector3 bot2 = WorldToScreen(Vector3(pos.x - bounds.x, pos.y + bounds.y, pos.z - bounds.z), global::cameracache);
//	Vector3 bot4 = WorldToScreen(Vector3(pos.x + bounds.x, pos.y - bounds.y, pos.z - bounds.z), global::cameracache);
//	Vector3 bot3 = WorldToScreen(Vector3(pos.x - bounds.x, pos.y - bounds.y, pos.z - bounds.z), global::cameracache);
//
//	std::list<std::pair<Vector3, Vector3>> topLines = { { top1,top2 },{ top2,top3 },{ top3,top4 },{ top4,top1 } };
//	std::list<std::pair<Vector3, Vector3>> botLines = { { bot1,bot2 },{ bot2,bot3 },{ bot3,bot4 },{ bot4,bot1 } };
//	std::list<std::pair<Vector3, Vector3>> sideLines = { { top1,bot1 },{ top2,bot2 },{ top3,bot3 },{ top4,bot4 } };
//	for (auto p : topLines)
//		DrawLine(p.first.x, p.first.y, p.second.x, p.second.y, D3DCOLOR_ARGB(255, 160, 32, 240));
//	for (auto p : botLines)
//		DrawLine(p.first.x, p.first.y, p.second.x, p.second.y, D3DCOLOR_ARGB(255, 160, 32, 240));
//	for (auto p : sideLines)
//		DrawLine(p.first.x, p.first.y, p.second.x, p.second.y, D3DCOLOR_ARGB(255, 160, 32, 240));
//}

void D3D11Renderer::DrawCircle(int x, int y, int radius, int sides, Color color)
{
	float Step = 3.14159265 * 2.0 / sides;
	for (float a = 0; a < 3.14159265 * 2.0; a += Step)
	{
		float X1 = radius * cos(a) + x;
		float Y1 = radius * sin(a) + y;
		float X2 = radius * cos(a + Step) + x;
		float Y2 = radius * sin(a + Step) + y;
		DrawLine(X1, Y1, X2, Y2, color);
	}
}

void D3D11Renderer::DrawHealthBar(float x, float y, float w, float health, float max)
{
	this->DrawHealthBar(x, y, w, 2, health, max);
}

void D3D11Renderer::DrawHealthBar(float x, float y, float w, float h, float health, float max)
{
	if (!max)
		return;

	if (w < 5)
		return;

	if (health < 0)
		health = 0;

	float ratio = health / max;

	Color col = Color(255, (uchar)(255 - 255 * ratio), (uchar)(255 * ratio), 0);

	float step = (w / max);
	float draw = (step * health);

	FillRect(x, y, w, h, Color(255, 0, 0, 0));
	FillRect(x, y, draw, h, col);
}

float D3D11Renderer::GetWidth()
{
	D3D11_VIEWPORT vp;
	UINT nvp = 1;
	this->deviceContext->RSGetViewports(&nvp, &vp);
	return vp.Width;
}

float D3D11Renderer::GetHeight()
{
	D3D11_VIEWPORT vp;
	UINT nvp = 1;
	this->deviceContext->RSGetViewports(&nvp, &vp);
	return vp.Height;
}

void D3D11Renderer::BeginScene()
{
	this->restoreState = false;
	if (SUCCEEDED(this->stateSaver->saveCurrentState(this->deviceContext)))
		this->restoreState = true;

	this->deviceContext->IASetInputLayout(this->mInputLayout);
}

void D3D11Renderer::EndScene()
{
	if (this->restoreState)
		this->stateSaver->restoreSavedState();
}