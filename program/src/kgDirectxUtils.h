#ifndef __KGDIRECTX_UTILS_H__
#define __KGDIRECTX_UTILS_H__
//----------------------------------------------------------------
// Compute shader ユーティリティ
//----------------------------------------------------------------

// 安全なリリース
inline void SafeRelease( IUnknown *p )
{
	if( p!=NULL ){
		p->Release();
	}
}

inline HRESULT CreateComputeShader( TCHAR				*csoName, 
									ID3D11Device		*pd3dDevice, 
									ID3D11ComputeShader	**resCS )
{
    //バイナリファイルを読み込む//
	FILE *fp;
	fopen_s( &fp, csoName, "rb");
	if( fp == NULL){
		return -1;//失敗
	}
	fseek(fp, 0, SEEK_END);		//終端まで行く。
	long cso_sz = ftell(fp);	//サイズを知る
	fseek(fp, 0, SEEK_SET);		//先頭に戻る
	
	unsigned char *cso_data = new unsigned char[ cso_sz ];
	fread( cso_data, cso_sz, 1, fp );    //コンパイル後のバイナリをcso_dataに格納
	fclose( fp );
	// シェーダーオブジェクトの作成
	ID3D11ComputeShader	*pComputeShader = NULL;
	HRESULT hr = pd3dDevice->CreateComputeShader( cso_data, cso_sz, NULL, &pComputeShader );
    *resCS = pComputeShader;
	delete [] cso_data;
    return hr;
}

// 構造体バッファの生成
inline HRESULT CreateStructuredBufferOnGPU(	ID3D11Device	*pDevice, 
											UINT			uElementSize, 
											UINT			uCount, 
											void			*pInitData, 
											ID3D11Buffer	**ppBufOut )
{
    *ppBufOut = NULL;
    D3D11_BUFFER_DESC desc;
    ZeroMemory( &desc, sizeof( desc ) );
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    desc.ByteWidth = uElementSize * uCount;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = uElementSize;

    if( pInitData ){
        D3D11_SUBRESOURCE_DATA InitData;
        InitData.pSysMem = pInitData;
        return pDevice->CreateBuffer( &desc, &InitData, ppBufOut );
    } else {
        return pDevice->CreateBuffer( &desc, NULL, ppBufOut );
	}
}

// シェーダリソースビューを構造体バッファか、そのままのバッファのために生成
inline HRESULT CreateBufferSRV(	ID3D11Device				*pDevice, 
								ID3D11Buffer				*pBuffer, 
								ID3D11ShaderResourceView	**ppSRVOut )
{
    D3D11_BUFFER_DESC descBuf;
    ZeroMemory( &descBuf, sizeof( descBuf ) );
    pBuffer->GetDesc( &descBuf );
    D3D11_SHADER_RESOURCE_VIEW_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
    desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
    desc.BufferEx.FirstElement = 0;

    if ( descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS ){
        // そのままのバッファ処理
        desc.Format					= DXGI_FORMAT_R32_TYPELESS;
        desc.BufferEx.Flags			= D3D11_BUFFEREX_SRV_FLAG_RAW;
        desc.BufferEx.NumElements	= descBuf.ByteWidth / 4;
    } else{
		if( descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED ){
			// 構造体バッファ処理
			desc.Format					= DXGI_FORMAT_UNKNOWN;
			desc.BufferEx.NumElements	= descBuf.ByteWidth / descBuf.StructureByteStride;
		}else{
			return E_INVALIDARG;
		}
	}
    return pDevice->CreateShaderResourceView( pBuffer, &desc, ppSRVOut );
}

// 構造体バッファ、またはそのままのバッファのためのUAVの生成
inline HRESULT CreateBufferUAV(	ID3D11Device				*pDevice, 
								ID3D11Buffer				*pBuffer, 
								ID3D11UnorderedAccessView	**ppUAVOut )
{
    D3D11_BUFFER_DESC descBuf;
    ZeroMemory( &descBuf, sizeof( descBuf ) );
    pBuffer->GetDesc( &descBuf );
        
    D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
    ZeroMemory( &desc, sizeof( desc ) );
    desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    desc.Buffer.FirstElement = 0;

    if ( descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS ){
        // This is a Raw Buffer
        desc.Format				= DXGI_FORMAT_R32_TYPELESS; 
		// Format must be DXGI_FORMAT_R32_TYPELESS, when creating Raw Unordered Access View
        desc.Buffer.Flags		= D3D11_BUFFER_UAV_FLAG_RAW;
        desc.Buffer.NumElements = descBuf.ByteWidth / 4; 
    } else {
		if ( descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED ){
			// This is a Structured Buffer
			desc.Format				= DXGI_FORMAT_UNKNOWN;      
			// Format must be must be DXGI_FORMAT_UNKNOWN, when creating a View of a Structured Buffer
			desc.Buffer.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride; 
		}else{
			return E_INVALIDARG;
		}
	}    
    return pDevice->CreateUnorderedAccessView( pBuffer, &desc, ppUAVOut );
}


// CPUがアクセスできるバッファを作成し、GPUバッファの内容をダウンロードする。
// この関数はCSプログラムのデバッグにとても便利
inline ID3D11Buffer* CreateAndCopyToCpuReadableMem(	ID3D11Device		*pDevice, 
													ID3D11DeviceContext *pd3dImmediateContext, 
													ID3D11Buffer		*pBuffer )
{
    ID3D11Buffer *buf = NULL;

    D3D11_BUFFER_DESC desc;
    ZeroMemory( &desc, sizeof( desc ) );
    pBuffer->GetDesc( &desc );
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.Usage			= D3D11_USAGE_STAGING;
    desc.BindFlags		= 0;
    desc.MiscFlags		= 0;
    pDevice->CreateBuffer( &desc, NULL, &buf );

    pd3dImmediateContext->CopyResource( buf, pBuffer );

    return buf;
}

// simpleBuffer 構造体
// GPUのものと同じフォーマットにする必要がある。

#endif