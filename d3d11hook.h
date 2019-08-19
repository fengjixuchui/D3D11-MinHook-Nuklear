#ifndef D3D11_HOOK_H_INCLUDED_
#define D3D11_HOOK_H_INCLUDED_

#define D3D11_HOOK_API

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;

D3D11_HOOK_API void ImplHookDX11_Present(ID3D11Device* device, ID3D11DeviceContext* ctx, IDXGISwapChain* swap_chain);

D3D11_HOOK_API void	       ImplHookDX11_Init(HMODULE hModule, void* hwnd);

D3D11_HOOK_API void	       ImplHookDX11_Shutdown();

#endif
