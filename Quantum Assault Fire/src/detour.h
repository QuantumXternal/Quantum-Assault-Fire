DWORD DetourCreate( DWORD  SrcVA, DWORD  DstVA, DWORD  Size )
{
	/*Credits for this detour to croner*/
#define SIZEOF_JMP_NEAR  5

	if ( SrcVA && DstVA && Size > 0 )
	{
		DWORD DetourVA = (DWORD) 
			VirtualAlloc( NULL, Size + SIZEOF_JMP_NEAR, 
			MEM_COMMIT | MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE );
		
		if ( DetourVA == 0 )
			return (0);

		DWORD dwProtect;
		if ( VirtualProtect( (VOID*)SrcVA, Size, PAGE_EXECUTE_READWRITE, &dwProtect ) )
		{
			for ( DWORD i=0; i < Size; i++ ) {
				*(BYTE*)( DetourVA + i ) = *(BYTE*)( SrcVA + i );
			}
			
			*(BYTE*)( DetourVA + Size + 0 ) = 0xE9;
			*(DWORD*)( DetourVA + Size + 1 ) = ( SrcVA - DetourVA - SIZEOF_JMP_NEAR );

	    	*(BYTE*)( SrcVA + 0 ) = 0xE9;
			*(DWORD*)( SrcVA + 1 ) = ( DstVA - SrcVA - SIZEOF_JMP_NEAR );

			VirtualProtect( (VOID*)SrcVA, Size, dwProtect, &dwProtect );

			VirtualProtect( (VOID*)DetourVA, Size + 
				SIZEOF_JMP_NEAR, PAGE_EXECUTE_READ, &dwProtect );

	    	return DetourVA;
		}
	}
	return (0);
}




PVOID D3Ddiscover(void *tbl, int size)
{

	
	HWND				  hWnd;
	void				  *pInterface=0 ;
	D3DPRESENT_PARAMETERS d3dpp; 

	if ((hWnd=CreateWindowEx(NULL,WC_DIALOG,"",WS_OVERLAPPED,0,0,50,50,NULL,NULL,NULL,NULL))==NULL) return 0;
	ShowWindow(hWnd, SW_HIDE);

	LPDIRECT3D9			pD3D;
	LPDIRECT3DDEVICE9	pD3Ddev;
	if ((pD3D = Direct3DCreate9(D3D_SDK_VERSION))!=NULL) 

	{
	    ZeroMemory(&d3dpp, sizeof(d3dpp));
	    d3dpp.Windowed         = TRUE;
		d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
	    d3dpp.hDeviceWindow    = hWnd;
	    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	    d3dpp.BackBufferWidth  = d3dpp.BackBufferHeight = 600;
		pD3D->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,hWnd,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&d3dpp,&pD3Ddev);
		if (pD3Ddev)  {
		    pInterface = (PDWORD)*(DWORD *)pD3Ddev;
			memcpy(tbl,(void *)pInterface,size);
			pD3Ddev->Release();
		}
		pD3D->Release();
	}
	DestroyWindow(hWnd);
	return pInterface;
}

void EraseHeaders(HINSTANCE hModule)
{
	/* 
	* jus a func to erase headers by Croner.
	* keep in mind you wont be able to load 
	* any resources after you erase headers.
	*/

	PIMAGE_DOS_HEADER pDoH; 
	PIMAGE_NT_HEADERS pNtH;
	DWORD i, ersize, protect;

	if (!hModule) return;
	
	// well just to make clear what we doing
	pDoH = (PIMAGE_DOS_HEADER)(hModule);

	pNtH = (PIMAGE_NT_HEADERS)((LONG)hModule + ((PIMAGE_DOS_HEADER)hModule)->e_lfanew);

	ersize = sizeof(IMAGE_DOS_HEADER);
	if ( VirtualProtect(pDoH, ersize, PAGE_READWRITE, &protect) )
	{
		for ( i=0; i < ersize; i++ )
				*(BYTE*)((BYTE*)pDoH + i) = 0;
	}

	ersize = sizeof(IMAGE_NT_HEADERS);
	if ( pNtH && VirtualProtect(pNtH, ersize, PAGE_READWRITE, &protect) )
	{
		for ( i=0; i < ersize; i++ )
				*(BYTE*)((BYTE*)pNtH + i) = 0;
	}
	return;
}


void HideModule(HINSTANCE hModule)/*Credits to Thanatos aka Surpintine for his hide module function*/
{
	DWORD dwPEB_LDR_DATA = 0;
	_asm
	{
		pushad;
		pushfd;
		mov eax, fs:[30h]             
		mov eax, [eax+0Ch]               
		mov dwPEB_LDR_DATA, eax	

		InLoadOrderModuleList:
			mov esi, [eax+0Ch]	     
			mov edx, [eax+10h]	     

		LoopInLoadOrderModuleList: 
		    lodsd		         
			mov esi, eax	
			mov ecx, [eax+18h]  
			cmp ecx, hModule	
			jne SkipA		 
		    mov ebx, [eax]	  
		    mov ecx, [eax+4]  
		    mov [ecx], ebx    
		    mov [ebx+4], ecx	  
			jmp InMemoryOrderModuleList 

		SkipA:
			cmp edx, esi     
			jne LoopInLoadOrderModuleList

		InMemoryOrderModuleList:
			mov eax, dwPEB_LDR_DATA
			mov esi, [eax+14h]
			mov edx, [eax+18h]

		LoopInMemoryOrderModuleList: 
			lodsd
			mov esi, eax
			mov ecx, [eax+10h]
			cmp ecx, hModule
			jne SkipB
			mov ebx, [eax] 
			mov ecx, [eax+4]
			mov [ecx], ebx
			mov [ebx+4], ecx
			jmp InInitializationOrderModuleList

		SkipB:
			cmp edx, esi
			jne LoopInMemoryOrderModuleList

		InInitializationOrderModuleList:
			mov eax, dwPEB_LDR_DATA
			mov esi, [eax+1Ch]	  
			mov edx, [eax+20h]	  

		LoopInInitializationOrderModuleList: 
			lodsd
			mov esi, eax		
			mov ecx, [eax+08h]
			cmp ecx, hModule		
			jne SkipC
			mov ebx, [eax] 
			mov ecx, [eax+4]
			mov [ecx], ebx
			mov [ebx+4], ecx
			jmp Finished

		SkipC:
			cmp edx, esi
			jne LoopInInitializationOrderModuleList

		Finished:
			popfd;
			popad;
	}
}

#define Detour_Type_0xE9  1
#define Detour_Type_0xB8  2
#define Detour_Type_0x68  3
DWORD CreateDetour(DWORD dwThread,DWORD dwAdress,DWORD dwType,DWORD dwSize)
{
        DWORD dwDetour,dwProtect,i;
        if (dwAdress&&dwThread&&dwSize>= dwSize)
        {
                dwDetour = (DWORD)VirtualAlloc(0,dwSize+dwSize,0x1000,0x40);
                if (dwDetour&&VirtualProtect((VOID*)dwAdress,dwSize,0x40,&dwProtect))
                {
                        for (i=0;i<dwSize;i++)
                        {
                                *(BYTE*)(dwDetour+i)=*(BYTE*)(dwAdress+i);
                        }
                        switch (dwType)
                        {
                            case Detour_Type_0xE9:
                                {
                                    *(BYTE*)(dwDetour+dwSize+0)=0xE9;
                                    *(DWORD*)(dwDetour+dwSize+1)=(dwAdress-dwDetour-dwSize);
                                    *(BYTE*)(dwAdress+0)=0xE9;
                                    *(DWORD*)(dwAdress+1)=(dwThread-dwAdress-dwSize);
                                }
                                break;
                                case Detour_Type_0xB8:
                                {
                                    *(BYTE*)(dwDetour+dwSize+0)=0xB8;
                                    *(DWORD*)(dwDetour+dwSize+1)=(dwAdress+dwSize);
                                    *(WORD*)(dwDetour+dwSize+5)=0xE0FF;
                                    *(BYTE*)(dwAdress+0)=0xB8;            
                                    *(DWORD*)(dwAdress+1)=(dwThread);
                                    *(WORD*)(dwAdress+5)=0xE0FF;
                                }
                                break;
                                case Detour_Type_0x68:
                                {
                                    *(BYTE*)(dwDetour+dwSize+0)=0x68;
                                    *(DWORD*)(dwDetour+dwSize+1)=(dwAdress+dwSize);
                                    *(WORD*)(dwDetour+dwSize+5)=0xC3;
                                    *(BYTE*)(dwAdress+0)=0x68;            
                                    *(DWORD*)(dwAdress+1)=(dwThread);
                                    *(WORD*)(dwAdress+5)=0xC3;
                                }
                                break;
                        }
                        VirtualProtect((VOID*)dwAdress,dwSize,dwProtect,&dwProtect);
                        VirtualProtect((VOID*)dwDetour,dwSize+dwSize,0x20,&dwProtect);
                        return dwDetour;
                }
        }
        Sleep(10);
        return (0);
}