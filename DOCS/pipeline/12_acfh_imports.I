#line 1 "TESTS/COMPILER/12_acfh_imports.ac"
#import draw_rect, clear_screen from "graphics.lib"
#import malloc, free from "libc.lib"
#line 4 "TESTS/COMPILER/12_acfh_imports.ac"
U32 main() {
draw_rect(10, 20, 100, 50, 0x00FF00);
clear_screen(0x000000);
U32 ptr = malloc(1024);
if(1==1) {
free(ptr);
return 1;
}
U32 ptr2 = malloc(670);
free(ptr);
free(ptr2);
return 0;
}
