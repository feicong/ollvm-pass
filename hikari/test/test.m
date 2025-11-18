#define TARGET_OS_OSX 1
#import <Foundation/Foundation.h>

@interface SimpleClass : NSObject
@property(nonatomic, strong) NSString *name;
- (void)hello;
@end

@implementation SimpleClass
- (void)hello {
    NSLog(@"Hello, %@", self.name);
}
@end

int main(int argc, char** argv) {
    if (argc > 1) {
        SimpleClass *obj = [SimpleClass new];
        obj.name = @"World";
        [obj hello];
    }
    return 0;
}
