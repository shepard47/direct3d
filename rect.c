#define COBJMACROS
#define CINTERFACE

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

/* -ldxgi -ld3d11 -ldxguid -ld3dcompiler -lgdi32 */

HINSTANCE inst;
WNDCLASSEX wc;
MSG e;
HWND win;
IDXGISwapChain *sc;
ID3D11Device *dev;
ID3D11DeviceContext *ctx;
ID3D11RenderTargetView *backbuf;
float col[] = { 0,0.2,0.4,1 };
ID3D11InputLayout *layout;
ID3D11VertexShader *vert;
ID3D11PixelShader *frag;
ID3D11Buffer *vbuf;
ID3D11Buffer* ibuf;

void
cleanup()
{
	ID3D11InputLayout_Release(layout);
	ID3D11VertexShader_Release(vert);
	ID3D11PixelShader_Release(frag);
	ID3D11Buffer_Release(vbuf);
	IDXGISwapChain_Release(sc);
	ID3D11RenderTargetView_Release(backbuf);
	ID3D11Device_Release(dev);
	ID3D11DeviceContext_Release(ctx);
}

LRESULT CALLBACK
winproc(HWND win, UINT etype, WPARAM wp, LPARAM lp)
{
	switch(etype){
	case WM_CLOSE:
		cleanup();
		DestroyWindow(win);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
	default:
		return DefWindowProc(win, etype, wp, lp);
	}
}

void
initd3d(void)
{
	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

	scd.BufferCount = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = win;
	scd.SampleDesc.Count = 4;
	scd.Windowed = 1;

	D3D11CreateDeviceAndSwapChain(0,D3D_DRIVER_TYPE_HARDWARE,0,0,0,0,D3D11_SDK_VERSION,&scd,&sc,&dev,0,&ctx);

	ID3D11Texture2D *pbackbuf;
	IDXGISwapChain_GetBuffer(sc, 0, &IID_ID3D11Texture2D, (void*)&pbackbuf);
	ID3D11Device_CreateRenderTargetView(dev, (ID3D11Resource*)pbackbuf, 0, &backbuf);
	ID3D11Texture2D_Release(pbackbuf);
	ID3D11DeviceContext_OMSetRenderTargets(ctx, 1, &backbuf, 0);

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = 800;
	viewport.Height = 600;
	ID3D11DeviceContext_RSSetViewports(ctx, 1, &viewport);
}

void
draw()
{
	ID3D11DeviceContext_ClearRenderTargetView(ctx, backbuf, col);
	ID3D11DeviceContext_VSSetShader(ctx, vert, 0, 0);
	ID3D11DeviceContext_PSSetShader(ctx, frag, 0, 0);

	unsigned int stride = sizeof(float) * 3;
	unsigned int istr = sizeof(int) * 3;
	unsigned int offset = 0;
	ID3D11DeviceContext_IASetVertexBuffers(ctx, 0, 1, &vbuf, &stride, &offset);
	ID3D11DeviceContext_IASetIndexBuffer(ctx, ibuf, DXGI_FORMAT_R32_UINT, 0);
	ID3D11DeviceContext_IASetPrimitiveTopology(ctx, D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ID3D11DeviceContext_DrawIndexed(ctx, 6, 0, 0);

	IDXGISwapChain_Present(sc, 0, 0);
}

char *vc = 
"struct VOut \n\
{ \n\
    float4 position : SV_POSITION; \n\
}; \n\
\n\
VOut vmain(float4 position : POSITION) \n\
{ \n\
    VOut output; \n\
 \n\
    output.position = position; \n\
 \n\
    return output; \n\
}";

char *fc = 
"float4 fmain() : SV_TARGET\n\
{\n\
    return float4(1.0f, 0.0f, 0.0f, 1.0f);\n\
}";

void
mkprog(void)
{
	ID3D10Blob *vs, *fs;

	D3DCompile(vc, strlen(vc), 0, 0, (ID3DInclude*)1, "vmain", "vs_4_0", 0, 0, &vs, 0);
	D3DCompile(fc, strlen(fc), 0, 0, (ID3DInclude*)1, "fmain", "ps_4_0", 0, 0, &fs, 0);

	ID3D11Device_CreateVertexShader(dev, ID3D10Blob_GetBufferPointer(vs), ID3D10Blob_GetBufferSize(vs), 0, &vert);
	ID3D11Device_CreatePixelShader(dev, ID3D10Blob_GetBufferPointer(fs), ID3D10Blob_GetBufferSize(fs), 0, &frag);
	ID3D11DeviceContext_VSSetShader(ctx, vert, 0, 0);
	ID3D11DeviceContext_PSSetShader(ctx, frag, 0, 0);

	D3D11_INPUT_ELEMENT_DESC ied[] = {
		{"POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}/*,
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},*/
	};

	ID3D11Device_CreateInputLayout(dev, ied, 1, ID3D10Blob_GetBufferPointer(vs), ID3D10Blob_GetBufferSize(vs), &layout);
	ID3D11DeviceContext_IASetInputLayout(ctx, layout);
}

void
mkrect()
{
	D3D11_BUFFER_DESC bd;
	D3D11_MAPPED_SUBRESOURCE ms;
	D3D11_SUBRESOURCE_DATA sr;

	float vdat[] = {
		 0.5f,  0.5f, 0.0f,
		 0.5f, -0.5f, 0.0f,
		-0.5f, -0.5f, 0.0f,
		-0.5f,  0.5f, 0.0f
	};
	unsigned int ind[] = {
		0, 1, 3,
		1, 2, 3
	};

	ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.ByteWidth = sizeof(ind);
	bd.CPUAccessFlags = 0;
	bd.Usage = D3D11_USAGE_DEFAULT;
	sr.pSysMem = ind;
	ID3D11Device_CreateBuffer(dev, &bd, &sr, &ibuf);

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(vdat);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	ID3D11Device_CreateBuffer(dev, &bd, 0, &vbuf);
	ID3D11DeviceContext_Map(ctx, (ID3D11Resource*)vbuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	memcpy(ms.pData, vdat, sizeof(vdat));
	ID3D11DeviceContext_Unmap(ctx, (ID3D11Resource*)vbuf, 0);
}

int
main()
{
	inst = GetModuleHandle(0);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = winproc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = inst;
	wc.hIcon = LoadIcon(0, IDI_WINLOGO);
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = "direct";
	wc.cbSize = sizeof(WNDCLASSEX);
	RegisterClassEx(&wc);

	win = CreateWindowEx(WS_EX_CLIENTEDGE, "direct", "direct", WS_OVERLAPPEDWINDOW, 0, 0, 800, 600, 0, 0, inst, 0);
	ShowWindow(win, SW_SHOW);
	UpdateWindow(win);

	initd3d();
	mkprog();
	mkrect();

	while(1){
		if(PeekMessage(&e, NULL, 0, 0, PM_REMOVE)){
			TranslateMessage(&e);
			DispatchMessage(&e);
		}
		draw();
	}

	return e.wParam;
}
