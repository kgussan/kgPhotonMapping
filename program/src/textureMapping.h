#ifndef __TEXTURE_MAPPING_H__
#define __TEXTURE_MAPPING_H__
#include <stdio.h>
#include <stdlib.h>
#include "kgstdint.h"
#include "types.h"
#include "kgtga.h"

inline Color GetTexMap(Intersection &intersection, Material &mat, nsTga::Texture_t &tex, FILE *fp=stdout)
{
	Color texColor;
	if(mat.m_texId != kTexIdNone){//テクスチャマッピング
		uint32_t u,v;//テクセル座標
		const bool debugTexMap=false;
		if(    (intersection.m_texCoord.x >= 0.0f ) && (intersection.m_texCoord.x <= 1.0f )
			&& (intersection.m_texCoord.y >= 0.0f ) && (intersection.m_texCoord.y <= 1.0f ) ){

			uint16_t id=	mat.m_texId;
			uint32_t width=	tex.width;
			uint32_t height=tex.height;
			uint32_t dim=	tex.bitPerPixel/8;
			uint8_t  *data=	tex.imageData;

			if(debugTexMap){
				if(dim!=3){
					fprintf(fp,"error tex dim=%d wh %d %d id %d \n",dim,width,height,id);
				}else{
					fprintf(fp,"correct tex dim=%d wh %d %d id %d \n",dim,width,height,id);
				}
			}
			u=(int)( width *intersection.m_texCoord.x );
			v=(int)( height*intersection.m_texCoord.y );
			texColor.x=(float)( data[ ( v*width+ u )*dim+0 ] ) / 255.0f;//8bit x 4 テクスチャのみしか受け付けない。
			texColor.y=(float)( data[ ( v*width+ u )*dim+1 ] ) / 255.0f;
			texColor.z=(float)( data[ ( v*width+ u )*dim+2 ] ) / 255.0f;
			if(debugTexMap)
				fprintf(fp,"typeid=%s "
							"u v = %d %d "
							"intersection.m_position.x y z=%3.2f %3.2f %3.2f "
							"intersection.m_texCoord.x y z=%3.4f %3.4f %3.2f success \n",
							typeid( *intersection.m_object ).name(),
							u,v,
							intersection.m_position.x,intersection.m_position.y,intersection.m_position.z,
							intersection.m_texCoord.x,intersection.m_texCoord.y,intersection.m_texCoord.z);
		}else if(0){//座標が失敗 clamp to border color
			texColor.x=(float)( 0.0f );
			texColor.y=(float)( 0.0f );
			texColor.z=(float)( 1.0f );//border color 色　青
			if(debugTexMap)
				fprintf(fp,"typeid=%s "
							"u v = %d %d "
							"intersection.m_position.x y z=%3.2f %3.2f %3.2f "
							"intersection.m_texCoord.x y z=%3.4f %3.4f %3.2f fault \n",
							typeid( *intersection.m_object ).name(),
							u,v,
							intersection.m_position.x,intersection.m_position.y,intersection.m_position.z,
							intersection.m_texCoord.x,intersection.m_texCoord.y,intersection.m_texCoord.z);
		}else{//座標が失敗 repeat
			uint16_t id=	mat.m_texId;
			uint32_t width=	tex.width;
			uint32_t height=tex.height;
			uint32_t dim=	tex.bitPerPixel/8;
			uint8_t  *data=	tex.imageData;
			u=(int)( width *intersection.m_texCoord.x )%width ;//repeat
			v=(int)( height*intersection.m_texCoord.y )%height;
			texColor.x=(float)( data[ ( v*width+ u )*dim+0 ] ) / 255.0f;//8bit x 4 テクスチャのみしか受け付けない。
			texColor.y=(float)( data[ ( v*width+ u )*dim+1 ] ) / 255.0f;
			texColor.z=(float)( data[ ( v*width+ u )*dim+2 ] ) / 255.0f;
		}
	}else{
		//テクスチャ指定のないときはマテリアル色を適用
		texColor=mat.m_color;
	}
	return texColor;
}
#endif 
