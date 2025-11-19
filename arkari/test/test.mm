#define TARGET_OS_OSX 1
#import <Foundation/Foundation.h>

int main(int argc, char** argv) {
    if (argc > 1) {
        NSLog(@"argnum: %d\n", argc);
    }
    return 0;
}
