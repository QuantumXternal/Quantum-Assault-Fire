#ifndef _D3DMENU_H
#define _D3DMENU_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3dx9.h>

#ifndef D3DFONT_RIGHT
#define D3DFONT_RIGHT		0x0008
#endif

#define 	MENUMAXITEMS	100
#define 	MENUFOLDER		1
#define 	MENUTEXT		2
#define 	MENUITEM		3
#define		MENUCAT			4

typedef struct {
  int  typ;		
  char *txt;	
  char **opt;	
  int  *var;	  
  int  maxvalue;
} tMENU;

class D3D9Menu
{
public:
	D3D9Menu(char *Title = 0)
	{
		Mtitle=Title;	 
		Mpos=0;			
		Mmax=0;		
		Mxofs =163.0f;	
		Mysize=16.0f;	
		Mvisible=1;		
		MENU=(tMENU **)malloc(4*MENUMAXITEMS);
		for (int i=0; i<MENUMAXITEMS; i++) MENU[i]=(tMENU *)malloc(sizeof(tMENU));
	}
	~D3D9Menu() {
		for (int i=0; i<MENUMAXITEMS; i++) free(MENU[i]);
		free(MENU);
	}

	int		Mmax;
	tMENU		**MENU;
	void AddItem(char *txt, char **opt, int *var, int maxvalue);
	void AddFolder(char *txt, char **opt, int *var, int maxvalue);
	void AddCategory(char *txt, char **opt, int *var, int maxvalue);
	void MenuShow(float x, float y,	CD3DFont*  pFont1, LPDIRECT3DDEVICE9 pDevice);
	void AddText(char *txt, char *opt);
	void MenuNav(void);
	void DrawTextR(int x,int y,DWORD color,char *text, CD3DFont*  pFont1);
	void DrawTextL(int x,int y,DWORD color,char *text, CD3DFont*  pFont1);
	void DrawTextC(int x,int y,DWORD color,char *text, CD3DFont*  pFont1);
	void DrawHealthBox( int x, int y, DWORD m_dColorOut, DWORD m_dColorIn, int m_iHealth, int m_iMaxHealth, IDirect3DDevice9* pDevice );
	//void DrawBorder( int x, int y, int w, int h, int px, D3DCOLOR BorderColor, IDirect3DDevice9* pDevice ); 


public:
	
	char	*Mtitle;
	int 	Mpos;
	float	Mxofs;	  
	float	Mysize;	 
	int		Mvisible;
	int		YPOS;
};
#endif
