# my_readelf

自作のreadelfコマンドです。64bitアーキテクチャ対応です。

elf実行ファイルのファイルヘッダ、セクションヘッダ、プログラムヘッダを出力します。

コマンドに下記のオプションをつけることで出力を変えれます。


-a[--all]             => 全ての情報を出力

-h[--file-header]     => ファイルヘッダを出力

-S[--section-headers] => セクションヘッダを出力

-l[--program-headers] => プログラムヘッダを表示

-H[--help]            => ヘルプを表示


# Usage

```bash
gcc main.c
./a.out (オプション) (elf実行ファイル)
```
