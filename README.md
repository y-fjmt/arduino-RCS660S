# arduino-RCS660S

## はじめに
SONYのRCS660/SモジュールをArduinoで利用するためのライブラリです。  
前モデルのRCS620/S用のライブラリの[arduino-RCS620S](http://blog.felicalauncher.com/sdk_for_air/?page_id=2699)と互換性を持たせているのでそのままご利用いただけます。
> [!NOTE]
> [arduino-RCS620S](http://blog.felicalauncher.com/sdk_for_air/?page_id=2699)での`push()`は非対応です。


## ライブラリ使用上のご注意
本ライブラリは、原則として入力値や内部状態の検査を行いません。
引数に正しくない値を指定した場合の動作は不定となりますので、
ご注意ください。
また、本プログラムの使用に起因する損害について、一切責任を負いませんので、ご了承ください。
