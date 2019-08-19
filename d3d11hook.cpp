#include "Nuklear/nuklear_d3d11.h"
nk_context* g_pNkContext;

#include "stdafx.h"

#include "MinHook/include/MinHook.h"
#include "FW1FontWrapper/FW1FontWrapper.h"

#include "d3d11hook.h"
#include <D3D11Renderer.h>

#pragma comment(lib, "d3d11.lib")

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <iostream>
#include <mutex>
#include <Globals.h>

typedef HRESULT(__stdcall* D3D11PresentHook) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef void(__stdcall* D3D11DrawIndexedHook) (ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);
typedef void(__stdcall* D3D11CreateQueryHook) (ID3D11Device* pDevice, const D3D11_QUERY_DESC* pQueryDesc, ID3D11Query** ppQuery);
typedef void(__stdcall* D3D11PSSetShaderResourcesHook) (ID3D11DeviceContext* pContext, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews);
typedef void(__stdcall* D3D11ClearRenderTargetViewHook) (ID3D11DeviceContext* pContext, ID3D11RenderTargetView* pRenderTargetView, const FLOAT ColorRGBA[4]);

static HWND                     g_hWnd = nullptr;
static HMODULE					g_hModule = nullptr;
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static std::once_flag           g_isInitialized;

D3D11PresentHook                phookD3D11Present = nullptr;
D3D11DrawIndexedHook            phookD3D11DrawIndexed = nullptr;
D3D11CreateQueryHook			phookD3D11CreateQuery = nullptr;
D3D11PSSetShaderResourcesHook	phookD3D11PSSetShaderResources = nullptr;
D3D11ClearRenderTargetViewHook  phookD3D11ClearRenderTargetViewHook = nullptr;

DWORD_PTR* pSwapChainVTable = nullptr;
DWORD_PTR* pDeviceVTable = nullptr;
DWORD_PTR* pDeviceContextVTable = nullptr;

D3D11Renderer* renderer;

D3D11_HOOK_API void ImplHookDX11_Present(ID3D11Device* device, ID3D11DeviceContext* ctx, IDXGISwapChain* swap_chain)
{
	renderer->DrawString(L"example string", 20, 30, 250, 0xff111611);
	renderer->DrawLine(400, 20, 20, 100, Color(255, 255, 0, 0));
	renderer->DrawBox(400, 200, 200, 400, Color(255, 0, 255, 0));
	renderer->DrawBox(450, 250, 250, 450, Color(255, 0, 0, 0));
	renderer->DrawHealthBar(400, 50, 100, 80, 100);
	renderer->DrawCircle(400, 100, 50, 30, Color(255, 0, 0, 0));
	renderer->FillRect(400, 100, 100, 200, Color(255, 1, 255, 0));

	/* window flags */
	static int show_menu = nk_true;
	static int titlebar = nk_true;
	static int border = nk_true;
	static int resize = nk_true;
	static int movable = nk_true;
	static int no_scrollbar = nk_false;
	static int scale_left = nk_false;
	static nk_flags window_flags = 0;
	static int minimizable = nk_true;

	/* popups */
	static enum nk_style_header_align header_align = NK_HEADER_RIGHT;
	static int show_app_about = nk_false;

	/* window flags */
	window_flags = 0;
	g_pNkContext->style.window.header.align = header_align;
	if (border) window_flags |= NK_WINDOW_BORDER;
	if (resize) window_flags |= NK_WINDOW_SCALABLE;
	if (movable) window_flags |= NK_WINDOW_MOVABLE;
	if (no_scrollbar) window_flags |= NK_WINDOW_NO_SCROLLBAR;
	if (scale_left) window_flags |= NK_WINDOW_SCALE_LEFT;
	if (minimizable) window_flags |= NK_WINDOW_MINIMIZABLE;

	if (nk_begin(g_pNkContext, "Overview", nk_rect(10, 10, 300, 400), window_flags))
	{
		if (show_app_about)
		{
			/* about popup */
			static struct nk_rect s = { 20, 100, 300, 190 };
			if (nk_popup_begin(g_pNkContext, NK_POPUP_STATIC, "About", NK_WINDOW_CLOSABLE, s))
			{
				nk_layout_row_dynamic(g_pNkContext, 20, 1);
				nk_label(g_pNkContext, "Nuklear", NK_TEXT_LEFT);
				nk_label(g_pNkContext, "By Micha Mettke", NK_TEXT_LEFT);
				nk_label(g_pNkContext, "nuklear is licensed under the public domain License.", NK_TEXT_LEFT);
				nk_popup_end(g_pNkContext);
			}
			else show_app_about = nk_false;
		}

		/* window flags */
		if (nk_tree_push(g_pNkContext, NK_TREE_TAB, "Window", NK_MINIMIZED)) {
			nk_layout_row_dynamic(g_pNkContext, 30, 2);
			nk_checkbox_label(g_pNkContext, "Titlebar", &titlebar);
			nk_checkbox_label(g_pNkContext, "Menu", &show_menu);
			nk_checkbox_label(g_pNkContext, "Border", &border);
			nk_checkbox_label(g_pNkContext, "Resizable", &resize);
			nk_checkbox_label(g_pNkContext, "Movable", &movable);
			nk_checkbox_label(g_pNkContext, "No Scrollbar", &no_scrollbar);
			nk_checkbox_label(g_pNkContext, "Minimizable", &minimizable);
			nk_checkbox_label(g_pNkContext, "Scale Left", &scale_left);
			nk_tree_pop(g_pNkContext);
		}

		if (nk_tree_push(g_pNkContext, NK_TREE_TAB, "Widgets", NK_MINIMIZED))
		{
			enum options { A, B, C };
			static int checkbox;
			static int option;
			if (nk_tree_push(g_pNkContext, NK_TREE_NODE, "Text", NK_MINIMIZED))
			{
				/* Text Widgets */
				nk_layout_row_dynamic(g_pNkContext, 20, 1);
				nk_label(g_pNkContext, "Label aligned left", NK_TEXT_LEFT);
				nk_label(g_pNkContext, "Label aligned centered", NK_TEXT_CENTERED);
				nk_label(g_pNkContext, "Label aligned right", NK_TEXT_RIGHT);
				nk_label_colored(g_pNkContext, "Blue text", NK_TEXT_LEFT, nk_rgb(0, 0, 255));
				nk_label_colored(g_pNkContext, "Yellow text", NK_TEXT_LEFT, nk_rgb(255, 255, 0));
				nk_text(g_pNkContext, "Text without /0", 15, NK_TEXT_RIGHT);

				nk_layout_row_static(g_pNkContext, 100, 200, 1);
				nk_label_wrap(g_pNkContext, "This is a very long line to hopefully get this text to be wrapped into multiple lines to show line wrapping");
				nk_layout_row_dynamic(g_pNkContext, 100, 1);
				nk_label_wrap(g_pNkContext, "This is another long text to show dynamic window changes on multiline text");
				nk_tree_pop(g_pNkContext);
			}

			if (nk_tree_push(g_pNkContext, NK_TREE_NODE, "Button", NK_MINIMIZED))
			{
				/* Buttons Widgets */
				nk_layout_row_static(g_pNkContext, 30, 100, 3);
				if (nk_button_label(g_pNkContext, "Button"))
					fprintf(stdout, "Button pressed!\n");
				nk_button_set_behavior(g_pNkContext, NK_BUTTON_REPEATER);
				if (nk_button_label(g_pNkContext, "Repeater"))
					fprintf(stdout, "Repeater is being pressed!\n");
				nk_button_set_behavior(g_pNkContext, NK_BUTTON_DEFAULT);
				nk_button_color(g_pNkContext, nk_rgb(0, 0, 255));

				nk_layout_row_static(g_pNkContext, 25, 25, 8);
				nk_button_symbol(g_pNkContext, NK_SYMBOL_CIRCLE_SOLID);
				nk_button_symbol(g_pNkContext, NK_SYMBOL_CIRCLE_OUTLINE);
				nk_button_symbol(g_pNkContext, NK_SYMBOL_RECT_SOLID);
				nk_button_symbol(g_pNkContext, NK_SYMBOL_RECT_OUTLINE);
				nk_button_symbol(g_pNkContext, NK_SYMBOL_TRIANGLE_UP);
				nk_button_symbol(g_pNkContext, NK_SYMBOL_TRIANGLE_DOWN);
				nk_button_symbol(g_pNkContext, NK_SYMBOL_TRIANGLE_LEFT);
				nk_button_symbol(g_pNkContext, NK_SYMBOL_TRIANGLE_RIGHT);

				nk_layout_row_static(g_pNkContext, 30, 100, 2);
				nk_button_symbol_label(g_pNkContext, NK_SYMBOL_TRIANGLE_LEFT, "prev", NK_TEXT_RIGHT);
				nk_button_symbol_label(g_pNkContext, NK_SYMBOL_TRIANGLE_RIGHT, "next", NK_TEXT_LEFT);
				nk_tree_pop(g_pNkContext);
			}

			if (nk_tree_push(g_pNkContext, NK_TREE_NODE, "Basic", NK_MINIMIZED))
			{
				/* Basic widgets */
				static int int_slider = 5;
				static float float_slider = 2.5f;
				static size_t prog_value = 40;
				static float property_float = 2;
				static int property_int = 10;
				static int property_neg = 10;

				static float range_float_min = 0;
				static float range_float_max = 100;
				static float range_float_value = 50;
				static int range_int_min = 0;
				static int range_int_value = 2048;
				static int range_int_max = 4096;
				static const float ratio[] = { 120, 150 };

				nk_layout_row_static(g_pNkContext, 30, 100, 1);
				nk_checkbox_label(g_pNkContext, "Checkbox", &checkbox);

				nk_layout_row_static(g_pNkContext, 30, 80, 3);
				option = nk_option_label(g_pNkContext, "optionA", option == A) ? A : option;
				option = nk_option_label(g_pNkContext, "optionB", option == B) ? B : option;
				option = nk_option_label(g_pNkContext, "optionC", option == C) ? C : option;

				nk_layout_row(g_pNkContext, NK_STATIC, 30, 2, ratio);
				nk_slider_int(g_pNkContext, 0, &int_slider, 10, 1);

				nk_label(g_pNkContext, "Slider float", NK_TEXT_LEFT);
				nk_slider_float(g_pNkContext, 0, &float_slider, 5.0, 0.5f);
				nk_progress(g_pNkContext, &prog_value, 100, NK_MODIFIABLE);

				nk_layout_row(g_pNkContext, NK_STATIC, 25, 2, ratio);
				nk_label(g_pNkContext, "Property float:", NK_TEXT_LEFT);
				nk_property_float(g_pNkContext, "Float:", 0, &property_float, 64.0f, 0.1f, 0.2f);
				nk_label(g_pNkContext, "Property int:", NK_TEXT_LEFT);
				nk_property_int(g_pNkContext, "Int:", 0, &property_int, 100.0f, 1, 1);
				nk_label(g_pNkContext, "Property neg:", NK_TEXT_LEFT);
				nk_property_int(g_pNkContext, "Neg:", -10, &property_neg, 10, 1, 1);

				nk_layout_row_dynamic(g_pNkContext, 25, 1);
				nk_label(g_pNkContext, "Range:", NK_TEXT_LEFT);
				nk_layout_row_dynamic(g_pNkContext, 25, 3);
				nk_property_float(g_pNkContext, "#min:", 0, &range_float_min, range_float_max, 1.0f, 0.2f);
				nk_property_float(g_pNkContext, "#float:", range_float_min, &range_float_value, range_float_max, 1.0f, 0.2f);
				nk_property_float(g_pNkContext, "#max:", range_float_min, &range_float_max, 100, 1.0f, 0.2f);

				nk_property_int(g_pNkContext, "#min:", INT_MIN, &range_int_min, range_int_max, 1, 10);
				nk_property_int(g_pNkContext, "#neg:", range_int_min, &range_int_value, range_int_max, 1, 10);
				nk_property_int(g_pNkContext, "#max:", range_int_min, &range_int_max, INT_MAX, 1, 10);

				nk_tree_pop(g_pNkContext);
			}

			if (nk_tree_push(g_pNkContext, NK_TREE_NODE, "Inactive", NK_MINIMIZED))
			{
				static int inactive = 1;
				nk_layout_row_dynamic(g_pNkContext, 30, 1);
				nk_checkbox_label(g_pNkContext, "Inactive", &inactive);

				nk_layout_row_static(g_pNkContext, 30, 80, 1);
				if (inactive) {
					struct nk_style_button button;
					button = g_pNkContext->style.button;
					g_pNkContext->style.button.normal = nk_style_item_color(nk_rgb(40, 40, 40));
					g_pNkContext->style.button.hover = nk_style_item_color(nk_rgb(40, 40, 40));
					g_pNkContext->style.button.active = nk_style_item_color(nk_rgb(40, 40, 40));
					g_pNkContext->style.button.border_color = nk_rgb(60, 60, 60);
					g_pNkContext->style.button.text_background = nk_rgb(60, 60, 60);
					g_pNkContext->style.button.text_normal = nk_rgb(60, 60, 60);
					g_pNkContext->style.button.text_hover = nk_rgb(60, 60, 60);
					g_pNkContext->style.button.text_active = nk_rgb(60, 60, 60);
					nk_button_label(g_pNkContext, "button");
					g_pNkContext->style.button = button;
				}
				else if (nk_button_label(g_pNkContext, "button"))
					fprintf(stdout, "button pressed\n");
				nk_tree_pop(g_pNkContext);
			}


			if (nk_tree_push(g_pNkContext, NK_TREE_NODE, "Selectable", NK_MINIMIZED))
			{
				if (nk_tree_push(g_pNkContext, NK_TREE_NODE, "List", NK_MINIMIZED))
				{
					static int selected[4] = { nk_false, nk_false, nk_true, nk_false };
					nk_layout_row_static(g_pNkContext, 18, 100, 1);
					nk_selectable_label(g_pNkContext, "Selectable", NK_TEXT_LEFT, &selected[0]);
					nk_selectable_label(g_pNkContext, "Selectable", NK_TEXT_LEFT, &selected[1]);
					nk_label(g_pNkContext, "Not Selectable", NK_TEXT_LEFT);
					nk_selectable_label(g_pNkContext, "Selectable", NK_TEXT_LEFT, &selected[2]);
					nk_selectable_label(g_pNkContext, "Selectable", NK_TEXT_LEFT, &selected[3]);
					nk_tree_pop(g_pNkContext);
				}
				if (nk_tree_push(g_pNkContext, NK_TREE_NODE, "Grid", NK_MINIMIZED))
				{
					int i;
					static int selected[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
					nk_layout_row_static(g_pNkContext, 50, 50, 4);
					for (i = 0; i < 16; ++i) {
						if (nk_selectable_label(g_pNkContext, "Z", NK_TEXT_CENTERED, &selected[i])) {
							int x = (i % 4), y = i / 4;
							if (x > 0) selected[i - 1] ^= 1;
							if (x < 3) selected[i + 1] ^= 1;
							if (y > 0) selected[i - 4] ^= 1;
							if (y < 3) selected[i + 4] ^= 1;
						}
					}
					nk_tree_pop(g_pNkContext);
				}
				nk_tree_pop(g_pNkContext);
			}

			if (nk_tree_push(g_pNkContext, NK_TREE_NODE, "Combo", NK_MINIMIZED))
			{
				static float chart_selection = 8.0f;
				static int current_weapon = 0;
				static int check_values[5];
				static float position[3];
				static struct nk_color combo_color = { 130, 50, 50, 255 };
				static struct nk_colorf combo_color2 = { 0.509f, 0.705f, 0.2f, 1.0f };
				static size_t prog_a = 20, prog_b = 40, prog_c = 10, prog_d = 90;
				static const char* weapons[] = { "Fist","Pistol","Shotgun","Plasma","BFG" };

				char buffer[64];
				size_t sum = 0;

				/* default combobox */
				nk_layout_row_static(g_pNkContext, 25, 200, 1);
				current_weapon = nk_combo(g_pNkContext, weapons, NK_LEN(weapons), current_weapon, 25, nk_vec2(200, 200));

				/* slider color combobox */
				if (nk_combo_begin_color(g_pNkContext, combo_color, nk_vec2(200, 200))) {
					float ratios[] = { 0.15f, 0.85f };
					nk_layout_row(g_pNkContext, NK_DYNAMIC, 30, 2, ratios);
					nk_label(g_pNkContext, "R:", NK_TEXT_LEFT);
					combo_color.r = (nk_byte)nk_slide_int(g_pNkContext, 0, combo_color.r, 255, 5);
					nk_label(g_pNkContext, "G:", NK_TEXT_LEFT);
					combo_color.g = (nk_byte)nk_slide_int(g_pNkContext, 0, combo_color.g, 255, 5);
					nk_label(g_pNkContext, "B:", NK_TEXT_LEFT);
					combo_color.b = (nk_byte)nk_slide_int(g_pNkContext, 0, combo_color.b, 255, 5);
					nk_label(g_pNkContext, "A:", NK_TEXT_LEFT);
					combo_color.a = (nk_byte)nk_slide_int(g_pNkContext, 0, combo_color.a, 255, 5);
					nk_combo_end(g_pNkContext);
				}
				/* complex color combobox */
				if (nk_combo_begin_color(g_pNkContext, nk_rgb_cf(combo_color2), nk_vec2(200, 400))) {
					enum color_mode { COL_RGB, COL_HSV };
					static int col_mode = COL_RGB;
#ifndef DEMO_DO_NOT_USE_COLOR_PICKER
					nk_layout_row_dynamic(g_pNkContext, 120, 1);
					combo_color2 = nk_color_picker(g_pNkContext, combo_color2, NK_RGBA);
#endif

					nk_layout_row_dynamic(g_pNkContext, 25, 2);
					col_mode = nk_option_label(g_pNkContext, "RGB", col_mode == COL_RGB) ? COL_RGB : col_mode;
					col_mode = nk_option_label(g_pNkContext, "HSV", col_mode == COL_HSV) ? COL_HSV : col_mode;

					nk_layout_row_dynamic(g_pNkContext, 25, 1);
					if (col_mode == COL_RGB) {
						combo_color2.r = nk_propertyf(g_pNkContext, "#R:", 0, combo_color2.r, 1.0f, 0.01f, 0.005f);
						combo_color2.g = nk_propertyf(g_pNkContext, "#G:", 0, combo_color2.g, 1.0f, 0.01f, 0.005f);
						combo_color2.b = nk_propertyf(g_pNkContext, "#B:", 0, combo_color2.b, 1.0f, 0.01f, 0.005f);
						combo_color2.a = nk_propertyf(g_pNkContext, "#A:", 0, combo_color2.a, 1.0f, 0.01f, 0.005f);
					}
					else {
						float hsva[4];
						nk_colorf_hsva_fv(hsva, combo_color2);
						hsva[0] = nk_propertyf(g_pNkContext, "#H:", 0, hsva[0], 1.0f, 0.01f, 0.05f);
						hsva[1] = nk_propertyf(g_pNkContext, "#S:", 0, hsva[1], 1.0f, 0.01f, 0.05f);
						hsva[2] = nk_propertyf(g_pNkContext, "#V:", 0, hsva[2], 1.0f, 0.01f, 0.05f);
						hsva[3] = nk_propertyf(g_pNkContext, "#A:", 0, hsva[3], 1.0f, 0.01f, 0.05f);
						combo_color2 = nk_hsva_colorfv(hsva);
					}
					nk_combo_end(g_pNkContext);
				}
				/* progressbar combobox */
				sum = prog_a + prog_b + prog_c + prog_d;
				sprintf(buffer, "%lu", sum);
				if (nk_combo_begin_label(g_pNkContext, buffer, nk_vec2(200, 200))) {
					nk_layout_row_dynamic(g_pNkContext, 30, 1);
					nk_progress(g_pNkContext, &prog_a, 100, NK_MODIFIABLE);
					nk_progress(g_pNkContext, &prog_b, 100, NK_MODIFIABLE);
					nk_progress(g_pNkContext, &prog_c, 100, NK_MODIFIABLE);
					nk_progress(g_pNkContext, &prog_d, 100, NK_MODIFIABLE);
					nk_combo_end(g_pNkContext);
				}

				/* checkbox combobox */
				sum = (size_t)(check_values[0] + check_values[1] + check_values[2] + check_values[3] + check_values[4]);
				sprintf(buffer, "%lu", sum);
				if (nk_combo_begin_label(g_pNkContext, buffer, nk_vec2(200, 200))) {
					nk_layout_row_dynamic(g_pNkContext, 30, 1);
					nk_checkbox_label(g_pNkContext, weapons[0], &check_values[0]);
					nk_checkbox_label(g_pNkContext, weapons[1], &check_values[1]);
					nk_checkbox_label(g_pNkContext, weapons[2], &check_values[2]);
					nk_checkbox_label(g_pNkContext, weapons[3], &check_values[3]);
					nk_combo_end(g_pNkContext);
				}

				/* complex text combobox */
				sprintf(buffer, "%.2f, %.2f, %.2f", position[0], position[1], position[2]);
				if (nk_combo_begin_label(g_pNkContext, buffer, nk_vec2(200, 200))) {
					nk_layout_row_dynamic(g_pNkContext, 25, 1);
					nk_property_float(g_pNkContext, "#X:", -1024.0f, &position[0], 1024.0f, 1, 0.5f);
					nk_property_float(g_pNkContext, "#Y:", -1024.0f, &position[1], 1024.0f, 1, 0.5f);
					nk_property_float(g_pNkContext, "#Z:", -1024.0f, &position[2], 1024.0f, 1, 0.5f);
					nk_combo_end(g_pNkContext);
				}

				/* chart combobox */
				sprintf(buffer, "%.1f", chart_selection);
				if (nk_combo_begin_label(g_pNkContext, buffer, nk_vec2(200, 250))) {
					size_t i = 0;
					static const float values[] = { 26.0f,13.0f,30.0f,15.0f,25.0f,10.0f,20.0f,40.0f, 12.0f, 8.0f, 22.0f, 28.0f, 5.0f };
					nk_layout_row_dynamic(g_pNkContext, 150, 1);
					nk_chart_begin(g_pNkContext, NK_CHART_COLUMN, NK_LEN(values), 0, 50);
					for (i = 0; i < NK_LEN(values); ++i) {
						nk_flags res = nk_chart_push(g_pNkContext, values[i]);
						if (res & NK_CHART_CLICKED) {
							chart_selection = values[i];
							nk_combo_close(g_pNkContext);
						}
					}
					nk_chart_end(g_pNkContext);
					nk_combo_end(g_pNkContext);
				}

				{
					static int time_selected = 0;
					static int date_selected = 0;
					static struct tm sel_time;
					static struct tm sel_date;
					if (!time_selected || !date_selected) {
						/* keep time and date updated if nothing is selected */
						time_t cur_time = time(0);
						struct tm* n = localtime(&cur_time);
						if (!time_selected)
							memcpy(&sel_time, n, sizeof(struct tm));
						if (!date_selected)
							memcpy(&sel_date, n, sizeof(struct tm));
					}

					/* time combobox */
					sprintf(buffer, "%02d:%02d:%02d", sel_time.tm_hour, sel_time.tm_min, sel_time.tm_sec);
					if (nk_combo_begin_label(g_pNkContext, buffer, nk_vec2(200, 250))) {
						time_selected = 1;
						nk_layout_row_dynamic(g_pNkContext, 25, 1);
						sel_time.tm_sec = nk_propertyi(g_pNkContext, "#S:", 0, sel_time.tm_sec, 60, 1, 1);
						sel_time.tm_min = nk_propertyi(g_pNkContext, "#M:", 0, sel_time.tm_min, 60, 1, 1);
						sel_time.tm_hour = nk_propertyi(g_pNkContext, "#H:", 0, sel_time.tm_hour, 23, 1, 1);
						nk_combo_end(g_pNkContext);
					}

					/* date combobox */
					sprintf(buffer, "%02d-%02d-%02d", sel_date.tm_mday, sel_date.tm_mon + 1, sel_date.tm_year + 1900);
					if (nk_combo_begin_label(g_pNkContext, buffer, nk_vec2(350, 400)))
					{
						int i = 0;
						const char* month[] = { "January", "February", "March",
							"April", "May", "June", "July", "August", "September",
							"October", "November", "December" };
						const char* week_days[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
						const int month_days[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
						int year = sel_date.tm_year + 1900;
						int leap_year = (!(year % 4) && ((year % 100))) || !(year % 400);
						int days = (sel_date.tm_mon == 1) ?
							month_days[sel_date.tm_mon] + leap_year :
							month_days[sel_date.tm_mon];

						/* header with month and year */
						date_selected = 1;
						nk_layout_row_begin(g_pNkContext, NK_DYNAMIC, 20, 3);
						nk_layout_row_push(g_pNkContext, 0.05f);
						if (nk_button_symbol(g_pNkContext, NK_SYMBOL_TRIANGLE_LEFT)) {
							if (sel_date.tm_mon == 0) {
								sel_date.tm_mon = 11;
								sel_date.tm_year = NK_MAX(0, sel_date.tm_year - 1);
							}
							else sel_date.tm_mon--;
						}
						nk_layout_row_push(g_pNkContext, 0.9f);
						sprintf(buffer, "%s %d", month[sel_date.tm_mon], year);
						nk_label(g_pNkContext, buffer, NK_TEXT_CENTERED);
						nk_layout_row_push(g_pNkContext, 0.05f);
						if (nk_button_symbol(g_pNkContext, NK_SYMBOL_TRIANGLE_RIGHT)) {
							if (sel_date.tm_mon == 11) {
								sel_date.tm_mon = 0;
								sel_date.tm_year++;
							}
							else sel_date.tm_mon++;
						}
						nk_layout_row_end(g_pNkContext);

						/* good old week day formula (double because precision) */
						{int year_n = (sel_date.tm_mon < 2) ? year - 1 : year;
						int y = year_n % 100;
						int c = year_n / 100;
						int y4 = (int)((float)y / 4);
						int c4 = (int)((float)c / 4);
						int m = (int)(2.6 * (double)(((sel_date.tm_mon + 10) % 12) + 1) - 0.2);
						int week_day = (((1 + m + y + y4 + c4 - 2 * c) % 7) + 7) % 7;

						/* weekdays  */
						nk_layout_row_dynamic(g_pNkContext, 35, 7);
						for (i = 0; i < (int)NK_LEN(week_days); ++i)
							nk_label(g_pNkContext, week_days[i], NK_TEXT_CENTERED);

						/* days  */
						if (week_day > 0) nk_spacing(g_pNkContext, week_day);
						for (i = 1; i <= days; ++i) {
							sprintf(buffer, "%d", i);
							if (nk_button_label(g_pNkContext, buffer)) {
								sel_date.tm_mday = i;
								nk_combo_close(g_pNkContext);
							}
						}}
						nk_combo_end(g_pNkContext);
					}
				}

				nk_tree_pop(g_pNkContext);
			}

			if (nk_tree_push(g_pNkContext, NK_TREE_NODE, "Input", NK_MINIMIZED))
			{
				static const float ratio[] = { 120, 150 };
				static char field_buffer[64];
				static char text[9][64];
				static int text_len[9];
				static char box_buffer[512];
				static int field_len;
				static int box_len;
				nk_flags active;

				nk_layout_row(g_pNkContext, NK_STATIC, 25, 2, ratio);
				nk_label(g_pNkContext, "Default:", NK_TEXT_LEFT);

				nk_edit_string(g_pNkContext, NK_EDIT_SIMPLE, text[0], &text_len[0], 64, nk_filter_default);
				nk_label(g_pNkContext, "Int:", NK_TEXT_LEFT);
				nk_edit_string(g_pNkContext, NK_EDIT_SIMPLE, text[1], &text_len[1], 64, nk_filter_decimal);
				nk_label(g_pNkContext, "Float:", NK_TEXT_LEFT);
				nk_edit_string(g_pNkContext, NK_EDIT_SIMPLE, text[2], &text_len[2], 64, nk_filter_float);
				nk_label(g_pNkContext, "Hex:", NK_TEXT_LEFT);
				nk_edit_string(g_pNkContext, NK_EDIT_SIMPLE, text[4], &text_len[4], 64, nk_filter_hex);
				nk_label(g_pNkContext, "Octal:", NK_TEXT_LEFT);
				nk_edit_string(g_pNkContext, NK_EDIT_SIMPLE, text[5], &text_len[5], 64, nk_filter_oct);
				nk_label(g_pNkContext, "Binary:", NK_TEXT_LEFT);
				nk_edit_string(g_pNkContext, NK_EDIT_SIMPLE, text[6], &text_len[6], 64, nk_filter_binary);

				nk_label(g_pNkContext, "Password:", NK_TEXT_LEFT);
				{
					int i = 0;
					int old_len = text_len[8];
					char buffer[64];
					for (i = 0; i < text_len[8]; ++i) buffer[i] = '*';
					nk_edit_string(g_pNkContext, NK_EDIT_FIELD, buffer, &text_len[8], 64, nk_filter_default);
					if (old_len < text_len[8])
						memcpy(&text[8][old_len], &buffer[old_len], (nk_size)(text_len[8] - old_len));
				}

				nk_label(g_pNkContext, "Field:", NK_TEXT_LEFT);
				nk_edit_string(g_pNkContext, NK_EDIT_FIELD, field_buffer, &field_len, 64, nk_filter_default);

				nk_label(g_pNkContext, "Box:", NK_TEXT_LEFT);
				nk_layout_row_static(g_pNkContext, 180, 278, 1);
				nk_edit_string(g_pNkContext, NK_EDIT_BOX, box_buffer, &box_len, 512, nk_filter_default);

				nk_layout_row(g_pNkContext, NK_STATIC, 25, 2, ratio);
				active = nk_edit_string(g_pNkContext, NK_EDIT_FIELD | NK_EDIT_SIG_ENTER, text[7], &text_len[7], 64, nk_filter_ascii);
				if (nk_button_label(g_pNkContext, "Submit") ||
					(active & NK_EDIT_COMMITED))
				{
					text[7][text_len[7]] = '\n';
					text_len[7]++;
					memcpy(&box_buffer[box_len], &text[7], (nk_size)text_len[7]);
					box_len += text_len[7];
					text_len[7] = 0;
				}
				nk_tree_pop(g_pNkContext);
			}
			nk_tree_pop(g_pNkContext);
		}

		if (nk_tree_push(g_pNkContext, NK_TREE_TAB, "Popup", NK_MINIMIZED))
		{
			static struct nk_color color = { 255,0,0, 255 };
			static int select[4];
			static int popup_active;
			const struct nk_input* in = &g_pNkContext->input;
			struct nk_rect bounds;

			/* menu contextual */
			nk_layout_row_static(g_pNkContext, 30, 160, 1);
			bounds = nk_widget_bounds(g_pNkContext);
			nk_label(g_pNkContext, "Right click me for menu", NK_TEXT_LEFT);

			if (nk_contextual_begin(g_pNkContext, 0, nk_vec2(100, 300), bounds)) {
				static size_t prog = 40;
				static int slider = 10;

				nk_layout_row_dynamic(g_pNkContext, 25, 1);
				nk_checkbox_label(g_pNkContext, "Menu", &show_menu);
				nk_progress(g_pNkContext, &prog, 100, NK_MODIFIABLE);
				nk_slider_int(g_pNkContext, 0, &slider, 16, 1);
				if (nk_contextual_item_label(g_pNkContext, "About", NK_TEXT_CENTERED))
					show_app_about = nk_true;
				nk_selectable_label(g_pNkContext, select[0] ? "Unselect" : "Select", NK_TEXT_LEFT, &select[0]);
				nk_selectable_label(g_pNkContext, select[1] ? "Unselect" : "Select", NK_TEXT_LEFT, &select[1]);
				nk_selectable_label(g_pNkContext, select[2] ? "Unselect" : "Select", NK_TEXT_LEFT, &select[2]);
				nk_selectable_label(g_pNkContext, select[3] ? "Unselect" : "Select", NK_TEXT_LEFT, &select[3]);
				nk_contextual_end(g_pNkContext);
			}

			/* color contextual */
			nk_layout_row_begin(g_pNkContext, NK_STATIC, 30, 2);
			nk_layout_row_push(g_pNkContext, 100);
			nk_label(g_pNkContext, "Right Click here:", NK_TEXT_LEFT);
			nk_layout_row_push(g_pNkContext, 50);
			bounds = nk_widget_bounds(g_pNkContext);
			nk_button_color(g_pNkContext, color);
			nk_layout_row_end(g_pNkContext);

			if (nk_contextual_begin(g_pNkContext, 0, nk_vec2(350, 60), bounds)) {
				nk_layout_row_dynamic(g_pNkContext, 30, 4);
				color.r = (nk_byte)nk_propertyi(g_pNkContext, "#r", 0, color.r, 255, 1, 1);
				color.g = (nk_byte)nk_propertyi(g_pNkContext, "#g", 0, color.g, 255, 1, 1);
				color.b = (nk_byte)nk_propertyi(g_pNkContext, "#b", 0, color.b, 255, 1, 1);
				color.a = (nk_byte)nk_propertyi(g_pNkContext, "#a", 0, color.a, 255, 1, 1);
				nk_contextual_end(g_pNkContext);
			}

			/* popup */
			nk_layout_row_begin(g_pNkContext, NK_STATIC, 30, 2);
			nk_layout_row_push(g_pNkContext, 100);
			nk_label(g_pNkContext, "Popup:", NK_TEXT_LEFT);
			nk_layout_row_push(g_pNkContext, 50);
			if (nk_button_label(g_pNkContext, "Popup"))
				popup_active = 1;
			nk_layout_row_end(g_pNkContext);

			if (popup_active)
			{
				static struct nk_rect s = { 20, 100, 220, 90 };
				if (nk_popup_begin(g_pNkContext, NK_POPUP_STATIC, "Error", 0, s))
				{
					nk_layout_row_dynamic(g_pNkContext, 25, 1);
					nk_label(g_pNkContext, "A terrible error as occured", NK_TEXT_LEFT);
					nk_layout_row_dynamic(g_pNkContext, 25, 2);
					if (nk_button_label(g_pNkContext, "OK")) {
						popup_active = 0;
						nk_popup_close(g_pNkContext);
					}
					if (nk_button_label(g_pNkContext, "Cancel")) {
						popup_active = 0;
						nk_popup_close(g_pNkContext);
					}
					nk_popup_end(g_pNkContext);
				}
				else popup_active = nk_false;
			}

			/* tooltip */
			nk_layout_row_static(g_pNkContext, 30, 150, 1);
			bounds = nk_widget_bounds(g_pNkContext);
			nk_label(g_pNkContext, "Hover me for tooltip", NK_TEXT_LEFT);
			if (nk_input_is_mouse_hovering_rect(in, bounds))
				nk_tooltip(g_pNkContext, "This is a tooltip");

			nk_tree_pop(g_pNkContext);
		}

		if (nk_tree_push(g_pNkContext, NK_TREE_TAB, "Layout", NK_MINIMIZED))
		{
			if (nk_tree_push(g_pNkContext, NK_TREE_NODE, "Widget", NK_MINIMIZED))
			{
				float ratio_two[] = { 0.2f, 0.6f, 0.2f };
				float width_two[] = { 100, 200, 50 };

				nk_layout_row_dynamic(g_pNkContext, 30, 1);
				nk_label(g_pNkContext, "Dynamic fixed column layout with generated position and size:", NK_TEXT_LEFT);
				nk_layout_row_dynamic(g_pNkContext, 30, 3);
				nk_button_label(g_pNkContext, "button");
				nk_button_label(g_pNkContext, "button");
				nk_button_label(g_pNkContext, "button");

				nk_layout_row_dynamic(g_pNkContext, 30, 1);
				nk_label(g_pNkContext, "static fixed column layout with generated position and size:", NK_TEXT_LEFT);
				nk_layout_row_static(g_pNkContext, 30, 100, 3);
				nk_button_label(g_pNkContext, "button");
				nk_button_label(g_pNkContext, "button");
				nk_button_label(g_pNkContext, "button");

				nk_layout_row_dynamic(g_pNkContext, 30, 1);
				nk_label(g_pNkContext, "Dynamic array-based custom column layout with generated position and custom size:", NK_TEXT_LEFT);
				nk_layout_row(g_pNkContext, NK_DYNAMIC, 30, 3, ratio_two);
				nk_button_label(g_pNkContext, "button");
				nk_button_label(g_pNkContext, "button");
				nk_button_label(g_pNkContext, "button");

				nk_layout_row_dynamic(g_pNkContext, 30, 1);
				nk_label(g_pNkContext, "Static array-based custom column layout with generated position and custom size:", NK_TEXT_LEFT);
				nk_layout_row(g_pNkContext, NK_STATIC, 30, 3, width_two);
				nk_button_label(g_pNkContext, "button");
				nk_button_label(g_pNkContext, "button");
				nk_button_label(g_pNkContext, "button");

				nk_layout_row_dynamic(g_pNkContext, 30, 1);
				nk_label(g_pNkContext, "Dynamic immediate mode custom column layout with generated position and custom size:", NK_TEXT_LEFT);
				nk_layout_row_begin(g_pNkContext, NK_DYNAMIC, 30, 3);
				nk_layout_row_push(g_pNkContext, 0.2f);
				nk_button_label(g_pNkContext, "button");
				nk_layout_row_push(g_pNkContext, 0.6f);
				nk_button_label(g_pNkContext, "button");
				nk_layout_row_push(g_pNkContext, 0.2f);
				nk_button_label(g_pNkContext, "button");
				nk_layout_row_end(g_pNkContext);

				nk_layout_row_dynamic(g_pNkContext, 30, 1);
				nk_label(g_pNkContext, "Static immediate mode custom column layout with generated position and custom size:", NK_TEXT_LEFT);
				nk_layout_row_begin(g_pNkContext, NK_STATIC, 30, 3);
				nk_layout_row_push(g_pNkContext, 100);
				nk_button_label(g_pNkContext, "button");
				nk_layout_row_push(g_pNkContext, 200);
				nk_button_label(g_pNkContext, "button");
				nk_layout_row_push(g_pNkContext, 50);
				nk_button_label(g_pNkContext, "button");
				nk_layout_row_end(g_pNkContext);

				nk_layout_row_dynamic(g_pNkContext, 30, 1);
				nk_label(g_pNkContext, "Static free space with custom position and custom size:", NK_TEXT_LEFT);
				nk_layout_space_begin(g_pNkContext, NK_STATIC, 60, 4);
				nk_layout_space_push(g_pNkContext, nk_rect(100, 0, 100, 30));
				nk_button_label(g_pNkContext, "button");
				nk_layout_space_push(g_pNkContext, nk_rect(0, 15, 100, 30));
				nk_button_label(g_pNkContext, "button");
				nk_layout_space_push(g_pNkContext, nk_rect(200, 15, 100, 30));
				nk_button_label(g_pNkContext, "button");
				nk_layout_space_push(g_pNkContext, nk_rect(100, 30, 100, 30));
				nk_button_label(g_pNkContext, "button");
				nk_layout_space_end(g_pNkContext);

				nk_layout_row_dynamic(g_pNkContext, 30, 1);
				nk_label(g_pNkContext, "Row template:", NK_TEXT_LEFT);
				nk_layout_row_template_begin(g_pNkContext, 30);
				nk_layout_row_template_push_dynamic(g_pNkContext);
				nk_layout_row_template_push_variable(g_pNkContext, 80);
				nk_layout_row_template_push_static(g_pNkContext, 80);
				nk_layout_row_template_end(g_pNkContext);
				nk_button_label(g_pNkContext, "button");
				nk_button_label(g_pNkContext, "button");
				nk_button_label(g_pNkContext, "button");

				nk_tree_pop(g_pNkContext);
			}

			if (nk_tree_push(g_pNkContext, NK_TREE_NODE, "Group", NK_MINIMIZED))
			{
				static int group_titlebar = nk_false;
				static int group_border = nk_true;
				static int group_no_scrollbar = nk_false;
				static int group_width = 320;
				static int group_height = 200;

				nk_flags group_flags = 0;
				if (group_border) group_flags |= NK_WINDOW_BORDER;
				if (group_no_scrollbar) group_flags |= NK_WINDOW_NO_SCROLLBAR;
				if (group_titlebar) group_flags |= NK_WINDOW_TITLE;

				nk_layout_row_dynamic(g_pNkContext, 30, 3);
				nk_checkbox_label(g_pNkContext, "Titlebar", &group_titlebar);
				nk_checkbox_label(g_pNkContext, "Border", &group_border);
				nk_checkbox_label(g_pNkContext, "No Scrollbar", &group_no_scrollbar);

				nk_layout_row_begin(g_pNkContext, NK_STATIC, 22, 3);
				nk_layout_row_push(g_pNkContext, 50);
				nk_label(g_pNkContext, "size:", NK_TEXT_LEFT);
				nk_layout_row_push(g_pNkContext, 130);
				nk_property_int(g_pNkContext, "#Width:", 100, &group_width, 500, 10, 1);
				nk_layout_row_push(g_pNkContext, 130);
				nk_property_int(g_pNkContext, "#Height:", 100, &group_height, 500, 10, 1);
				nk_layout_row_end(g_pNkContext);

				nk_layout_row_static(g_pNkContext, (float)group_height, group_width, 2);
				if (nk_group_begin(g_pNkContext, "Group", group_flags)) {
					int i = 0;
					static int selected[16];
					nk_layout_row_static(g_pNkContext, 18, 100, 1);
					for (i = 0; i < 16; ++i)
						nk_selectable_label(g_pNkContext, (selected[i]) ? "Selected" : "Unselected", NK_TEXT_CENTERED, &selected[i]);
					nk_group_end(g_pNkContext);
				}
				nk_tree_pop(g_pNkContext);
			}
			if (nk_tree_push(g_pNkContext, NK_TREE_NODE, "Tree", NK_MINIMIZED))
			{
				static int root_selected = 0;
				int sel = root_selected;
				if (nk_tree_element_push(g_pNkContext, NK_TREE_NODE, "Root", NK_MINIMIZED, &sel)) {
					static int selected[8];
					int i = 0, node_select = selected[0];
					if (sel != root_selected) {
						root_selected = sel;
						for (i = 0; i < 8; ++i)
							selected[i] = sel;
					}
					if (nk_tree_element_push(g_pNkContext, NK_TREE_NODE, "Node", NK_MINIMIZED, &node_select)) {
						int j = 0;
						static int sel_nodes[4];
						if (node_select != selected[0]) {
							selected[0] = node_select;
							for (i = 0; i < 4; ++i)
								sel_nodes[i] = node_select;
						}
						nk_layout_row_static(g_pNkContext, 18, 100, 1);
						for (j = 0; j < 4; ++j)
							nk_selectable_symbol_label(g_pNkContext, NK_SYMBOL_CIRCLE_SOLID, (sel_nodes[j]) ? "Selected" : "Unselected", NK_TEXT_RIGHT, &sel_nodes[j]);
						nk_tree_element_pop(g_pNkContext);
					}
					nk_layout_row_static(g_pNkContext, 18, 100, 1);
					for (i = 1; i < 8; ++i)
						nk_selectable_symbol_label(g_pNkContext, NK_SYMBOL_CIRCLE_SOLID, (selected[i]) ? "Selected" : "Unselected", NK_TEXT_RIGHT, &selected[i]);
					nk_tree_element_pop(g_pNkContext);
				}
				nk_tree_pop(g_pNkContext);
			}
			if (nk_tree_push(g_pNkContext, NK_TREE_NODE, "Notebook", NK_MINIMIZED))
			{
				static int current_tab = 0;
				struct nk_rect bounds;
				float step = (2 * 3.141592654f) / 32;
				enum chart_type { CHART_LINE, CHART_HISTO, CHART_MIXED };
				const char* names[] = { "Lines", "Columns", "Mixed" };
				float id = 0;
				int i;

				/* Header */
				nk_style_push_vec2(g_pNkContext, &g_pNkContext->style.window.spacing, nk_vec2(0, 0));
				nk_style_push_float(g_pNkContext, &g_pNkContext->style.button.rounding, 0);
				nk_layout_row_begin(g_pNkContext, NK_STATIC, 20, 3);
				for (i = 0; i < 3; ++i) {
					/* make sure button perfectly fits text */
					const struct nk_user_font* f = g_pNkContext->style.font;
					float text_width = f->width(f->userdata, f->height, names[i], nk_strlen(names[i]));
					float widget_width = text_width + 3 * g_pNkContext->style.button.padding.x;
					nk_layout_row_push(g_pNkContext, widget_width);
					if (current_tab == i) {
						/* active tab gets highlighted */
						struct nk_style_item button_color = g_pNkContext->style.button.normal;
						g_pNkContext->style.button.normal = g_pNkContext->style.button.active;
						current_tab = nk_button_label(g_pNkContext, names[i]) ? i : current_tab;
						g_pNkContext->style.button.normal = button_color;
					}
					else current_tab = nk_button_label(g_pNkContext, names[i]) ? i : current_tab;
				}
				nk_style_pop_float(g_pNkContext);

				/* Body */
				nk_layout_row_dynamic(g_pNkContext, 140, 1);
				if (nk_group_begin(g_pNkContext, "Notebook", NK_WINDOW_BORDER))
				{
					nk_style_pop_vec2(g_pNkContext);
					switch (current_tab) {
					default: break;
					case CHART_LINE:
						nk_layout_row_dynamic(g_pNkContext, 100, 1);
						bounds = nk_widget_bounds(g_pNkContext);
						if (nk_chart_begin_colored(g_pNkContext, NK_CHART_LINES, nk_rgb(255, 0, 0), nk_rgb(150, 0, 0), 32, 0.0f, 1.0f)) {
							nk_chart_add_slot_colored(g_pNkContext, NK_CHART_LINES, nk_rgb(0, 0, 255), nk_rgb(0, 0, 150), 32, -1.0f, 1.0f);
							for (i = 0, id = 0; i < 32; ++i) {
								nk_chart_push_slot(g_pNkContext, (float)fabs(sin(id)), 0);
								nk_chart_push_slot(g_pNkContext, (float)cos(id), 1);
								id += step;
							}
						}
						nk_chart_end(g_pNkContext);
						break;
					case CHART_HISTO:
						nk_layout_row_dynamic(g_pNkContext, 100, 1);
						bounds = nk_widget_bounds(g_pNkContext);
						if (nk_chart_begin_colored(g_pNkContext, NK_CHART_COLUMN, nk_rgb(255, 0, 0), nk_rgb(150, 0, 0), 32, 0.0f, 1.0f)) {
							for (i = 0, id = 0; i < 32; ++i) {
								nk_chart_push_slot(g_pNkContext, (float)fabs(sin(id)), 0);
								id += step;
							}
						}
						nk_chart_end(g_pNkContext);
						break;
					case CHART_MIXED:
						nk_layout_row_dynamic(g_pNkContext, 100, 1);
						bounds = nk_widget_bounds(g_pNkContext);
						if (nk_chart_begin_colored(g_pNkContext, NK_CHART_LINES, nk_rgb(255, 0, 0), nk_rgb(150, 0, 0), 32, 0.0f, 1.0f)) {
							nk_chart_add_slot_colored(g_pNkContext, NK_CHART_LINES, nk_rgb(0, 0, 255), nk_rgb(0, 0, 150), 32, -1.0f, 1.0f);
							nk_chart_add_slot_colored(g_pNkContext, NK_CHART_COLUMN, nk_rgb(0, 255, 0), nk_rgb(0, 150, 0), 32, 0.0f, 1.0f);
							for (i = 0, id = 0; i < 32; ++i) {
								nk_chart_push_slot(g_pNkContext, (float)fabs(sin(id)), 0);
								nk_chart_push_slot(g_pNkContext, (float)fabs(cos(id)), 1);
								nk_chart_push_slot(g_pNkContext, (float)fabs(sin(id)), 2);
								id += step;
							}
						}
						nk_chart_end(g_pNkContext);
						break;
					}
					nk_group_end(g_pNkContext);
				}
				else nk_style_pop_vec2(g_pNkContext);
				nk_tree_pop(g_pNkContext);
			}

			if (nk_tree_push(g_pNkContext, NK_TREE_NODE, "Simple", NK_MINIMIZED))
			{
				nk_layout_row_dynamic(g_pNkContext, 300, 2);
				if (nk_group_begin(g_pNkContext, "Group_Without_Border", 0)) {
					int i = 0;
					char buffer[64];
					nk_layout_row_static(g_pNkContext, 18, 150, 1);
					for (i = 0; i < 64; ++i) {
						sprintf(buffer, "0x%02x", i);
					}
					nk_group_end(g_pNkContext);
				}
				if (nk_group_begin(g_pNkContext, "Group_With_Border", NK_WINDOW_BORDER)) {
					int i = 0;
					char buffer[64];
					nk_layout_row_dynamic(g_pNkContext, 25, 2);
					for (i = 0; i < 64; ++i) {
						sprintf(buffer, "%08d", ((((i % 7) * 10) ^ 32)) + (64 + (i % 2) * 2));
						nk_button_label(g_pNkContext, buffer);
					}
					nk_group_end(g_pNkContext);
				}
				nk_tree_pop(g_pNkContext);
			}

			if (nk_tree_push(g_pNkContext, NK_TREE_NODE, "Complex", NK_MINIMIZED))
			{
				int i;
				nk_layout_space_begin(g_pNkContext, NK_STATIC, 500, 64);
				nk_layout_space_push(g_pNkContext, nk_rect(0, 0, 150, 500));
				if (nk_group_begin(g_pNkContext, "Group_left", NK_WINDOW_BORDER)) {
					static int selected[32];
					nk_layout_row_static(g_pNkContext, 18, 100, 1);
					for (i = 0; i < 32; ++i)
						nk_selectable_label(g_pNkContext, (selected[i]) ? "Selected" : "Unselected", NK_TEXT_CENTERED, &selected[i]);
					nk_group_end(g_pNkContext);
				}

				nk_layout_space_push(g_pNkContext, nk_rect(160, 0, 150, 240));
				if (nk_group_begin(g_pNkContext, "Group_top", NK_WINDOW_BORDER)) {
					nk_layout_row_dynamic(g_pNkContext, 25, 1);
					nk_button_label(g_pNkContext, "#FFAA");
					nk_button_label(g_pNkContext, "#FFBB");
					nk_button_label(g_pNkContext, "#FFCC");
					nk_button_label(g_pNkContext, "#FFDD");
					nk_button_label(g_pNkContext, "#FFEE");
					nk_button_label(g_pNkContext, "#FFFF");
					nk_group_end(g_pNkContext);
				}

				nk_layout_space_push(g_pNkContext, nk_rect(160, 250, 150, 250));
				if (nk_group_begin(g_pNkContext, "Group_buttom", NK_WINDOW_BORDER)) {
					nk_layout_row_dynamic(g_pNkContext, 25, 1);
					nk_button_label(g_pNkContext, "#FFAA");
					nk_button_label(g_pNkContext, "#FFBB");
					nk_button_label(g_pNkContext, "#FFCC");
					nk_button_label(g_pNkContext, "#FFDD");
					nk_button_label(g_pNkContext, "#FFEE");
					nk_button_label(g_pNkContext, "#FFFF");
					nk_group_end(g_pNkContext);
				}

				nk_layout_space_push(g_pNkContext, nk_rect(320, 0, 150, 150));
				if (nk_group_begin(g_pNkContext, "Group_right_top", NK_WINDOW_BORDER)) {
					static int selected[4];
					nk_layout_row_static(g_pNkContext, 18, 100, 1);
					for (i = 0; i < 4; ++i)
						nk_selectable_label(g_pNkContext, (selected[i]) ? "Selected" : "Unselected", NK_TEXT_CENTERED, &selected[i]);
					nk_group_end(g_pNkContext);
				}

				nk_layout_space_push(g_pNkContext, nk_rect(320, 160, 150, 150));
				if (nk_group_begin(g_pNkContext, "Group_right_center", NK_WINDOW_BORDER)) {
					static int selected[4];
					nk_layout_row_static(g_pNkContext, 18, 100, 1);
					for (i = 0; i < 4; ++i)
						nk_selectable_label(g_pNkContext, (selected[i]) ? "Selected" : "Unselected", NK_TEXT_CENTERED, &selected[i]);
					nk_group_end(g_pNkContext);
				}

				nk_layout_space_push(g_pNkContext, nk_rect(320, 320, 150, 150));
				if (nk_group_begin(g_pNkContext, "Group_right_bottom", NK_WINDOW_BORDER)) {
					static int selected[4];
					nk_layout_row_static(g_pNkContext, 18, 100, 1);
					for (i = 0; i < 4; ++i)
						nk_selectable_label(g_pNkContext, (selected[i]) ? "Selected" : "Unselected", NK_TEXT_CENTERED, &selected[i]);
					nk_group_end(g_pNkContext);
				}
				nk_layout_space_end(g_pNkContext);
				nk_tree_pop(g_pNkContext);
			}

			if (nk_tree_push(g_pNkContext, NK_TREE_NODE, "Splitter", NK_MINIMIZED))
			{
				const struct nk_input* in = &g_pNkContext->input;
				nk_layout_row_static(g_pNkContext, 20, 320, 1);
				nk_label(g_pNkContext, "Use slider and spinner to change tile size", NK_TEXT_LEFT);
				nk_label(g_pNkContext, "Drag the space between tiles to change tile ratio", NK_TEXT_LEFT);

				if (nk_tree_push(g_pNkContext, NK_TREE_NODE, "Vertical", NK_MINIMIZED))
				{
					static float a = 100, b = 100, c = 100;
					struct nk_rect bounds;

					float row_layout[5];
					row_layout[0] = a;
					row_layout[1] = 8;
					row_layout[2] = b;
					row_layout[3] = 8;
					row_layout[4] = c;

					/* header */
					nk_layout_row_static(g_pNkContext, 30, 100, 2);
					nk_label(g_pNkContext, "left:", NK_TEXT_LEFT);
					nk_slider_float(g_pNkContext, 10.0f, &a, 200.0f, 10.0f);

					nk_label(g_pNkContext, "middle:", NK_TEXT_LEFT);
					nk_slider_float(g_pNkContext, 10.0f, &b, 200.0f, 10.0f);

					nk_label(g_pNkContext, "right:", NK_TEXT_LEFT);
					nk_slider_float(g_pNkContext, 10.0f, &c, 200.0f, 10.0f);

					/* tiles */
					nk_layout_row(g_pNkContext, NK_STATIC, 200, 5, row_layout);

					/* left space */
					if (nk_group_begin(g_pNkContext, "left", NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {
						nk_layout_row_dynamic(g_pNkContext, 25, 1);
						nk_button_label(g_pNkContext, "#FFAA");
						nk_button_label(g_pNkContext, "#FFBB");
						nk_button_label(g_pNkContext, "#FFCC");
						nk_button_label(g_pNkContext, "#FFDD");
						nk_button_label(g_pNkContext, "#FFEE");
						nk_button_label(g_pNkContext, "#FFFF");
						nk_group_end(g_pNkContext);
					}

					/* scaler */
					bounds = nk_widget_bounds(g_pNkContext);
					nk_spacing(g_pNkContext, 1);
					if ((nk_input_is_mouse_hovering_rect(in, bounds) ||
						nk_input_is_mouse_prev_hovering_rect(in, bounds)) &&
						nk_input_is_mouse_down(in, NK_BUTTON_LEFT))
					{
						a = row_layout[0] + in->mouse.delta.x;
						b = row_layout[2] - in->mouse.delta.x;
					}

					/* middle space */
					if (nk_group_begin(g_pNkContext, "center", NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {
						nk_layout_row_dynamic(g_pNkContext, 25, 1);
						nk_button_label(g_pNkContext, "#FFAA");
						nk_button_label(g_pNkContext, "#FFBB");
						nk_button_label(g_pNkContext, "#FFCC");
						nk_button_label(g_pNkContext, "#FFDD");
						nk_button_label(g_pNkContext, "#FFEE");
						nk_button_label(g_pNkContext, "#FFFF");
						nk_group_end(g_pNkContext);
					}

					/* scaler */
					bounds = nk_widget_bounds(g_pNkContext);
					nk_spacing(g_pNkContext, 1);
					if ((nk_input_is_mouse_hovering_rect(in, bounds) ||
						nk_input_is_mouse_prev_hovering_rect(in, bounds)) &&
						nk_input_is_mouse_down(in, NK_BUTTON_LEFT))
					{
						b = (row_layout[2] + in->mouse.delta.x);
						c = (row_layout[4] - in->mouse.delta.x);
					}

					/* right space */
					if (nk_group_begin(g_pNkContext, "right", NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {
						nk_layout_row_dynamic(g_pNkContext, 25, 1);
						nk_button_label(g_pNkContext, "#FFAA");
						nk_button_label(g_pNkContext, "#FFBB");
						nk_button_label(g_pNkContext, "#FFCC");
						nk_button_label(g_pNkContext, "#FFDD");
						nk_button_label(g_pNkContext, "#FFEE");
						nk_button_label(g_pNkContext, "#FFFF");
						nk_group_end(g_pNkContext);
					}

					nk_tree_pop(g_pNkContext);
				}

				if (nk_tree_push(g_pNkContext, NK_TREE_NODE, "Horizontal", NK_MINIMIZED))
				{
					static float a = 100, b = 100, c = 100;
					struct nk_rect bounds;

					/* header */
					nk_layout_row_static(g_pNkContext, 30, 100, 2);
					nk_label(g_pNkContext, "top:", NK_TEXT_LEFT);
					nk_slider_float(g_pNkContext, 10.0f, &a, 200.0f, 10.0f);

					nk_label(g_pNkContext, "middle:", NK_TEXT_LEFT);
					nk_slider_float(g_pNkContext, 10.0f, &b, 200.0f, 10.0f);

					nk_label(g_pNkContext, "bottom:", NK_TEXT_LEFT);
					nk_slider_float(g_pNkContext, 10.0f, &c, 200.0f, 10.0f);

					/* top space */
					nk_layout_row_dynamic(g_pNkContext, a, 1);
					if (nk_group_begin(g_pNkContext, "top", NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {
						nk_layout_row_dynamic(g_pNkContext, 25, 3);
						nk_button_label(g_pNkContext, "#FFAA");
						nk_button_label(g_pNkContext, "#FFBB");
						nk_button_label(g_pNkContext, "#FFCC");
						nk_button_label(g_pNkContext, "#FFDD");
						nk_button_label(g_pNkContext, "#FFEE");
						nk_button_label(g_pNkContext, "#FFFF");
						nk_group_end(g_pNkContext);
					}

					/* scaler */
					nk_layout_row_dynamic(g_pNkContext, 8, 1);
					bounds = nk_widget_bounds(g_pNkContext);
					nk_spacing(g_pNkContext, 1);
					if ((nk_input_is_mouse_hovering_rect(in, bounds) ||
						nk_input_is_mouse_prev_hovering_rect(in, bounds)) &&
						nk_input_is_mouse_down(in, NK_BUTTON_LEFT))
					{
						a = a + in->mouse.delta.y;
						b = b - in->mouse.delta.y;
					}

					/* middle space */
					nk_layout_row_dynamic(g_pNkContext, b, 1);
					if (nk_group_begin(g_pNkContext, "middle", NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {
						nk_layout_row_dynamic(g_pNkContext, 25, 3);
						nk_button_label(g_pNkContext, "#FFAA");
						nk_button_label(g_pNkContext, "#FFBB");
						nk_button_label(g_pNkContext, "#FFCC");
						nk_button_label(g_pNkContext, "#FFDD");
						nk_button_label(g_pNkContext, "#FFEE");
						nk_button_label(g_pNkContext, "#FFFF");
						nk_group_end(g_pNkContext);
					}

					{
						/* scaler */
						nk_layout_row_dynamic(g_pNkContext, 8, 1);
						bounds = nk_widget_bounds(g_pNkContext);
						if ((nk_input_is_mouse_hovering_rect(in, bounds) ||
							nk_input_is_mouse_prev_hovering_rect(in, bounds)) &&
							nk_input_is_mouse_down(in, NK_BUTTON_LEFT))
						{
							b = b + in->mouse.delta.y;
							c = c - in->mouse.delta.y;
						}
					}

					/* bottom space */
					nk_layout_row_dynamic(g_pNkContext, c, 1);
					if (nk_group_begin(g_pNkContext, "bottom", NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {
						nk_layout_row_dynamic(g_pNkContext, 25, 3);
						nk_button_label(g_pNkContext, "#FFAA");
						nk_button_label(g_pNkContext, "#FFBB");
						nk_button_label(g_pNkContext, "#FFCC");
						nk_button_label(g_pNkContext, "#FFDD");
						nk_button_label(g_pNkContext, "#FFEE");
						nk_button_label(g_pNkContext, "#FFFF");
						nk_group_end(g_pNkContext);
					}
					nk_tree_pop(g_pNkContext);
				}
				nk_tree_pop(g_pNkContext);
			}
			nk_tree_pop(g_pNkContext);
		}
	}

	nk_end(g_pNkContext);

	nk_d3d11_render(g_pd3dContext, NK_ANTI_ALIASING_OFF);
}

HRESULT __stdcall PresentHook(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	std::call_once(g_isInitialized, [&]() {
		pSwapChain->GetDevice(__uuidof(g_pd3dDevice), reinterpret_cast<void**>(&g_pd3dDevice));
		g_pd3dDevice->GetImmediateContext(&g_pd3dContext);

		renderer = new D3D11Renderer(pSwapChain);
		renderer->Initialize();

		D3D11_VIEWPORT vpOld;
		UINT nViewPorts = 1;

		g_pd3dContext->RSGetViewports(&nViewPorts, &vpOld);
		Globals::g_iWindowWidth = vpOld.Width;
		Globals::g_iWindowHeight = vpOld.Height;

		g_pNkContext = nk_d3d11_init(g_pd3dDevice, Globals::g_iWindowWidth, Globals::g_iWindowHeight, MAX_VERTEX_BUFFER, MAX_INDEX_BUFFER);

		nk_font_atlas* pNkAtlas;
		nk_d3d11_font_stash_begin(&pNkAtlas);
		nk_d3d11_font_stash_end();

		set_style(g_pNkContext, THEME_BLACK);
		});

	MSG msg;
	nk_input_begin(g_pNkContext);
	while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
	{
		nk_d3d11_handle_event(g_hWnd, msg.message, msg.wParam, msg.lParam);
	}
	nk_input_end(g_pNkContext);

	renderer->BeginScene();
	ImplHookDX11_Present(g_pd3dDevice, g_pd3dContext, g_pSwapChain);
	renderer->EndScene();

	return phookD3D11Present(pSwapChain, SyncInterval, Flags);
}

void __stdcall DrawIndexedHook(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
	return phookD3D11DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
}

void __stdcall hookD3D11CreateQuery(ID3D11Device* pDevice, const D3D11_QUERY_DESC* pQueryDesc, ID3D11Query** ppQuery)
{
	if (pQueryDesc->Query == D3D11_QUERY_OCCLUSION)
	{
		D3D11_QUERY_DESC oqueryDesc = CD3D11_QUERY_DESC();
		(&oqueryDesc)->MiscFlags = pQueryDesc->MiscFlags;
		(&oqueryDesc)->Query = D3D11_QUERY_TIMESTAMP;

		return phookD3D11CreateQuery(pDevice, &oqueryDesc, ppQuery);
	}

	return phookD3D11CreateQuery(pDevice, pQueryDesc, ppQuery);
}

UINT pssrStartSlot;
D3D11_SHADER_RESOURCE_VIEW_DESC Descr;

void __stdcall hookD3D11PSSetShaderResources(ID3D11DeviceContext* pContext, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews)
{
	pssrStartSlot = StartSlot;

	for (UINT j = 0; j < NumViews; j++)
	{
		ID3D11ShaderResourceView* pShaderResView = ppShaderResourceViews[j];
		if (pShaderResView)
		{
			pShaderResView->GetDesc(&Descr);

			if ((Descr.ViewDimension == D3D11_SRV_DIMENSION_BUFFER) || (Descr.ViewDimension == D3D11_SRV_DIMENSION_BUFFEREX))
			{
				continue; //Skip buffer resources
			}
		}
	}

	return phookD3D11PSSetShaderResources(pContext, StartSlot, NumViews, ppShaderResourceViews);
}

void __stdcall ClearRenderTargetViewHook(ID3D11DeviceContext* pContext, ID3D11RenderTargetView* pRenderTargetView, const FLOAT ColorRGBA[4])
{
	return phookD3D11ClearRenderTargetViewHook(pContext, pRenderTargetView, ColorRGBA);
}

DWORD __stdcall HookDX11_Init()
{
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

#pragma region Initialise DXGI_SWAP_CHAIN_DESC
	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(scd));

	scd.BufferCount = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	scd.OutputWindow = g_hWnd;
	scd.SampleDesc.Count = 1;
	scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	scd.BufferDesc.Width = 100;
	scd.BufferDesc.Height = 100;
	scd.BufferDesc.RefreshRate.Numerator = 244;
	scd.BufferDesc.RefreshRate.Denominator = 1;
#pragma endregion

	if (FAILED(D3D11CreateDeviceAndSwapChain(
		NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
		NULL, &featureLevel, 1, D3D11_SDK_VERSION,
		&scd, &g_pSwapChain,
		&g_pd3dDevice, NULL, &g_pd3dContext
	)))
	{
		return 0;
	}

	pSwapChainVTable = (DWORD_PTR*)(g_pSwapChain);
	pSwapChainVTable = (DWORD_PTR*)(pSwapChainVTable[0]);

	pDeviceVTable = (DWORD_PTR*)(g_pd3dDevice);
	pDeviceVTable = (DWORD_PTR*)pDeviceVTable[0];

	pDeviceContextVTable = (DWORD_PTR*)(g_pd3dContext);
	pDeviceContextVTable = (DWORD_PTR*)(pDeviceContextVTable[0]);

	if (MH_Initialize() != MH_OK) { return 1; }

	if (MH_CreateHook((DWORD_PTR*)pSwapChainVTable[8], PresentHook, reinterpret_cast<void**>(&phookD3D11Present)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pSwapChainVTable[8]) != MH_OK) { return 1; }

	if (MH_CreateHook((DWORD_PTR*)pDeviceContextVTable[12], DrawIndexedHook, reinterpret_cast<void**>(&phookD3D11DrawIndexed)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pDeviceContextVTable[12]) != MH_OK) { return 1; }

	if (MH_CreateHook((DWORD_PTR*)pDeviceVTable[24], hookD3D11CreateQuery, reinterpret_cast<void**>(&phookD3D11CreateQuery)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pDeviceVTable[24]) != MH_OK) { return 1; }

	if (MH_CreateHook((DWORD_PTR*)pDeviceContextVTable[8], hookD3D11PSSetShaderResources, reinterpret_cast<void**>(&phookD3D11PSSetShaderResources)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pDeviceContextVTable[8]) != MH_OK) { return 1; }

	if (MH_CreateHook((DWORD_PTR*)pSwapChainVTable[50], ClearRenderTargetViewHook, reinterpret_cast<void**>(&phookD3D11ClearRenderTargetViewHook)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)pSwapChainVTable[50]) != MH_OK) { return 1; }

	DWORD old_protect;
	VirtualProtect(phookD3D11Present, 2, PAGE_EXECUTE_READWRITE, &old_protect);

	do {
		Sleep(100);
	} while (!(GetAsyncKeyState(VK_END) & 0x1));

	nk_d3d11_shutdown();

	g_pd3dDevice->Release();
	g_pd3dContext->Release();
	g_pSwapChain->Release();

	ImplHookDX11_Shutdown();

	FreeLibraryAndExitThread(g_hModule, 0);

	return S_OK;
}

D3D11_HOOK_API void ImplHookDX11_Init(HMODULE hModule, void* hwnd)
{
	g_hWnd = (HWND)hwnd;
	g_hModule = hModule;
	HookDX11_Init();
}

D3D11_HOOK_API void ImplHookDX11_Shutdown()
{
	if (MH_DisableHook(MH_ALL_HOOKS)) { return; };
	if (MH_Uninitialize()) { return; }
}