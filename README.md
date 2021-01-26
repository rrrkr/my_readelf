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

# DEMO

ファイルヘッダ出力

![スクリーンショット 2021-01-26 12 42 12](https://user-images.githubusercontent.com/47289623/105797870-3c0b2180-5fd4-11eb-8eda-be11aab61211.png)

プログラムヘッダ出力

![スクリーンショット 2021-01-26 12 43 13](https://user-images.githubusercontent.com/47289623/105797882-42010280-5fd4-11eb-9eb4-d3eef9808f3c.png)

セクションヘッダ出力

![スクリーンショット 2021-01-26 12 43 52](https://user-images.githubusercontent.com/47289623/105797887-45948980-5fd4-11eb-92b0-176c4c57a4c9.png)
