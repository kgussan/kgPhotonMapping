kgPhotonMapping
kgussan@gmail.com
2014/09/13
Copyright(c) 2014 KGussan
================================================================

HLSL Compute shaderのシンプルなフォトンマッピングのGPU実装です。
1レイ1スレッドでフォトントレーシングを行い、
1ピクセル1スレッドでフォトンサンプリング、放射輝度計算を行います。

日本語のプログラム説明文書があります。
slide share
http://www.slideshare.net/ssuser2e676d/kg-photonmapping-v010

intel ノートPCなどではGPU動作が不安定になることがあります。
ドライバが失敗することもあるかもしれませんが、
待っているとwindows が復帰するはずです。

hlslはfxcコンパイラを使ってバイナリ形式に変更してください。
ランタイムコンパイルはうまくいきません。

----------------------------------------------------------------
ライセンス
MITライセンスに準じます。
一部のコードは同じくMITライセンスのgitholeさんのコードを基にしています。
このソフトウェアに関する損害は補償されません。
ライセンス関係の文言を残せば再配布・改変が可能です。

This code is released under MIT license.
A part of the code is based on githole photon mapping code
which is also under the same license.

