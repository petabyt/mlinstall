.global favicon_ico
.global favicon_ico_length
.align 4
favicon_ico:
.incbin "assets/mlinstall.png"
.align 4
favicon_ico_length:
.int favicon_ico_length - favicon_ico
