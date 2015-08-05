/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2015 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#if SDL_VIDEO_DRIVER_UIKIT

#include "SDL_video.h"
#include "SDL_assert.h"
#include "SDL_hints.h"
#include "../SDL_sysvideo.h"
#include "../../events/SDL_events_c.h"

#import "SDL_uikitviewcontroller.h"
#import "SDL_uikitmessagebox.h"
#include "SDL_uikitvideo.h"
#include "SDL_uikitmodes.h"
#include "SDL_uikitwindow.h"

#if SDL_IPHONE_KEYBOARD
#include "keyinfotable.h"
int keyboardHeight2 = 0;
#endif

#import <AVFoundation/AVFoundation.h>
#import <CoreMotion/CoreMotion.h>

static const char* kRecordAudioFile = NULL;

typedef void (* fn_3axis_sensor_callback)(double x, double y, double z);
static fn_3axis_sensor_callback acceleration_callback = NULL;
static fn_3axis_sensor_callback gyroscope_callback = NULL;

@interface SDL_uikitviewcontroller ()<AVAudioRecorderDelegate>
@property (nonatomic, strong) AVAudioRecorder *audioRecorder;
@property (nonatomic, strong) CMMotionManager *motionManager;


@end

@implementation SDL_uikitviewcontroller {
    CADisplayLink *displayLink;
    int animationInterval;
    void (*animationCallback)(void*);
    void *animationCallbackParam;

#if SDL_IPHONE_KEYBOARD
    UITextField *textField;
#endif
}

@synthesize window;

- (instancetype)initWithSDLWindow:(SDL_Window *)_window
{
    if (self = [super initWithNibName:nil bundle:nil]) {
        self.window = _window;

#if SDL_IPHONE_KEYBOARD
        [self initKeyboard];
#endif
    }
    return self;
}

- (void)dealloc
{
#if SDL_IPHONE_KEYBOARD
    [self deinitKeyboard];
#endif
}

- (void)setAnimationCallback:(int)interval
                    callback:(void (*)(void*))callback
               callbackParam:(void*)callbackParam
{
    [self stopAnimation];

    animationInterval = interval;
    animationCallback = callback;
    animationCallbackParam = callbackParam;

    if (animationCallback) {
        [self startAnimation];
    }
}

- (void)startAnimation
{
    displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(doLoop:)];
    [displayLink setFrameInterval:animationInterval];
    [displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
}

- (void)stopAnimation
{
    [displayLink invalidate];
    displayLink = nil;
}

- (void)doLoop:(CADisplayLink*)sender
{
    /* Don't run the game loop while a messagebox is up */
    if (!UIKit_ShowingMessageBox()) {
        animationCallback(animationCallbackParam);
    }
}

- (void)loadView
{
    /* Do nothing. */
}

- (void)viewDidLayoutSubviews
{
    const CGSize size = self.view.bounds.size;
    int w = (int) size.width;
    int h = (int) size.height;

    SDL_SendWindowEvent(window, SDL_WINDOWEVENT_RESIZED, w, h);
}

- (NSUInteger)supportedInterfaceOrientations
{
	// Fix bug temporary.
	if (window->x || window->y || window->w > 1024 || window->h > 1024 || window->min_w || window->min_h || window->max_w || window->max_h || window->hit_test || window->hit_test_data || window->prev || window->next) {
        NSUInteger orientationMask = 0;
        UIInterfaceOrientation curorient = [UIApplication sharedApplication].statusBarOrientation;
        if (UIInterfaceOrientationIsLandscape(curorient)) {
            return UIInterfaceOrientationMaskLandscapeLeft | UIInterfaceOrientationMaskLandscapeRight;
        } else {
            return UIInterfaceOrientationMaskPortrait;
        }
	}
    return UIKit_GetSupportedOrientations(window);
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)orient
{
    return ([self supportedInterfaceOrientations] & (1 << orient)) != 0;
}

- (BOOL)prefersStatusBarHidden
{
    return (window->flags & (SDL_WINDOW_FULLSCREEN|SDL_WINDOW_BORDERLESS)) != 0;
}

- (UIStatusBarStyle)preferredStatusBarStyle
{
    /* We assume most SDL apps don't have a bright white background. */
    return UIStatusBarStyleLightContent;
}

/*
 ---- Keyboard related functionality below this line ----
 */
#if SDL_IPHONE_KEYBOARD

@synthesize textInputRect;
@synthesize keyboardHeight;
@synthesize keyboardVisible;

/* Set ourselves up as a UITextFieldDelegate */
- (void)initKeyboard
{
    textField = [[UITextField alloc] initWithFrame:CGRectZero];
    textField.delegate = self;
    /* placeholder so there is something to delete! */
    textField.text = @" ";

    /* set UITextInputTrait properties, mostly to defaults */
    textField.autocapitalizationType = UITextAutocapitalizationTypeNone;
    textField.autocorrectionType = UITextAutocorrectionTypeNo;
    textField.enablesReturnKeyAutomatically = NO;
    textField.keyboardAppearance = UIKeyboardAppearanceDefault;
    textField.keyboardType = UIKeyboardTypeDefault;
    textField.returnKeyType = UIReturnKeyDefault;
    textField.secureTextEntry = NO;

    textField.hidden = YES;
    keyboardVisible = NO;

    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    [center addObserver:self selector:@selector(keyboardWillShow:) name:UIKeyboardWillShowNotification object:nil];
    [center addObserver:self selector:@selector(keyboardWillHide:) name:UIKeyboardWillHideNotification object:nil];
}

- (void)setView:(UIView *)view
{
    [super setView:view];

    [view addSubview:textField];

    if (keyboardVisible) {
        [self showKeyboard];
    }
}

- (void)deinitKeyboard
{
    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    [center removeObserver:self name:UIKeyboardWillShowNotification object:nil];
    [center removeObserver:self name:UIKeyboardWillHideNotification object:nil];
}

/* reveal onscreen virtual keyboard */
- (void)showKeyboard
{
    keyboardVisible = YES;
    if (textField.window) {
        [textField becomeFirstResponder];
    }
}

/* hide onscreen virtual keyboard */
- (void)hideKeyboard
{
    keyboardVisible = NO;
    [textField resignFirstResponder];
}

- (void)keyboardWillShow:(NSNotification *)notification
{
    CGRect kbrect = [[notification userInfo][UIKeyboardFrameBeginUserInfoKey] CGRectValue];

    /* The keyboard rect is in the coordinate space of the screen/window, but we
     * want its height in the coordinate space of the view. */
    kbrect = [self.view convertRect:kbrect fromView:nil];

    [self setKeyboardHeight:(int)kbrect.size.height];
    {
		// special kingdom
		CGSize keyboardEndSize = [[[notification userInfo] objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue].size;
		keyboardHeight2 = keyboardEndSize.height;
		SDL_SendKeyboardKey(SDL_PRESSED, SDL_SCANCODE_PRINTSCREEN);
	}
}

- (void)keyboardWillHide:(NSNotification *)notification
{
    [self setKeyboardHeight:0];
}

- (void)updateKeyboard
{
    CGAffineTransform t = self.view.transform;
    CGPoint offset = CGPointMake(0.0, 0.0);
    CGRect frame = UIKit_ComputeViewFrame(window, self.view.window.screen);

    if (self.keyboardHeight) {
        int rectbottom = self.textInputRect.y + self.textInputRect.h;
        int keybottom = self.view.bounds.size.height - self.keyboardHeight;
        if (keybottom < rectbottom) {
            offset.y = keybottom - rectbottom;
        }
    }

    /* Apply this view's transform (except any translation) to the offset, in
     * order to orient it correctly relative to the frame's coordinate space. */
    t.tx = 0.0;
    t.ty = 0.0;
    offset = CGPointApplyAffineTransform(offset, t);

    /* Apply the updated offset to the view's frame. */
    frame.origin.x += offset.x;
    frame.origin.y += offset.y;

	// don't scroll application window
    // self.view.frame = frame;
}

- (void)setKeyboardHeight:(int)height
{
    keyboardVisible = height > 0;
    keyboardHeight = height;
    [self updateKeyboard];
}

/* UITextFieldDelegate method.  Invoked when user types something. */
- (BOOL)textField:(UITextField *)_textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
    NSUInteger len = string.length;

    if (len == 0) {
        /* it wants to replace text with nothing, ie a delete */
        SDL_SendKeyboardKey(SDL_PRESSED, SDL_SCANCODE_BACKSPACE);
        SDL_SendKeyboardKey(SDL_RELEASED, SDL_SCANCODE_BACKSPACE);
    } else {
        /* go through all the characters in the string we've been sent and
         * convert them to key presses */
        int i;
        for (i = 0; i < len; i++) {
            unichar c = [string characterAtIndex:i];
            Uint16 mod = 0;
            SDL_Scancode code;

            if (c < 127) {
                /* figure out the SDL_Scancode and SDL_keymod for this unichar */
                code = unicharToUIKeyInfoTable[c].code;
                mod  = unicharToUIKeyInfoTable[c].mod;
            } else {
                /* we only deal with ASCII right now */
                code = SDL_SCANCODE_UNKNOWN;
                mod = 0;
            }

            if (mod & KMOD_SHIFT) {
                /* If character uses shift, press shift down */
                SDL_SendKeyboardKey(SDL_PRESSED, SDL_SCANCODE_LSHIFT);
            }

            /* send a keydown and keyup even for the character */
            SDL_SendKeyboardKey(SDL_PRESSED, code);
            SDL_SendKeyboardKey(SDL_RELEASED, code);

            if (mod & KMOD_SHIFT) {
                /* If character uses shift, press shift back up */
                SDL_SendKeyboardKey(SDL_RELEASED, SDL_SCANCODE_LSHIFT);
            }
        }

        SDL_SendKeyboardText([string UTF8String]);
    }

    return NO; /* don't allow the edit! (keep placeholder text there) */
}

/* Terminates the editing session */
- (BOOL)textFieldShouldReturn:(UITextField*)_textField
{
    SDL_SendKeyboardKey(SDL_PRESSED, SDL_SCANCODE_RETURN);
    SDL_SendKeyboardKey(SDL_RELEASED, SDL_SCANCODE_RETURN);
    SDL_StopTextInput();
    return YES;
}

#endif

/**
 *  取得录音文件保存路径
 *
 *  @return 录音文件路径
 */
- (NSURL *)getSavePath
{
    NSString *urlStr = [[NSString alloc] initWithCString:(const char*)kRecordAudioFile encoding:NSASCIIStringEncoding];
/*
	NSString *_kRecordAudioFile = [[NSString alloc] initWithCString:(const char*)kRecordAudioFile encoding:NSASCIIStringEncoding];

    NSString *urlStr=[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject];
    urlStr=[urlStr stringByAppendingPathComponent:_kRecordAudioFile];
*/
    NSLog(@"file path:%@",urlStr);
    NSURL *url=[NSURL fileURLWithPath:urlStr];
    return url;
}

/**
 *  获得录音机对象
 *
 *  @return 录音机对象
 */
- (AVAudioRecorder *)audioRecorder
{
    if (!_audioRecorder) {
        //创建录音文件保存路径
        NSURL *url=[self getSavePath];
        //创建录音格式设置
        
        NSMutableDictionary *setting = [[NSMutableDictionary alloc] initWithCapacity:10];
		[setting setObject:[NSNumber numberWithInt: kAudioFormatLinearPCM] forKey: AVFormatIDKey];
		[setting setObject:[NSNumber numberWithFloat:8000.0] forKey: AVSampleRateKey];
		[setting setObject:[NSNumber numberWithInt:1] forKey:AVNumberOfChannelsKey];
		[setting setObject:[NSNumber numberWithInt:8] forKey:AVLinearPCMBitDepthKey];
		[setting setObject:[NSNumber numberWithBool:NO] forKey:AVLinearPCMIsBigEndianKey];
		[setting setObject:[NSNumber numberWithBool:NO] forKey:AVLinearPCMIsFloatKey];
		
        //创建录音机
        NSError *error=nil;
        _audioRecorder=[[AVAudioRecorder alloc]initWithURL:url settings:setting error:&error];
        _audioRecorder.delegate=self;
        _audioRecorder.meteringEnabled=YES;//如果要监控声波则必须设置为YES
        if (error) {
            NSLog(@"创建录音机对象时发生错误，错误信息：%@",error.localizedDescription);
            return nil;
        }
    }
    return _audioRecorder;
}

/**
 *  录音完成，录音完成后播放录音
 *
 *  @param recorder 录音机对象
 *  @param flag     是否成功
 */
- (void)audioRecorderDidFinishRecording:(AVAudioRecorder *)recorder successfully:(BOOL)flag
{
    NSLog(@"%@",NSStringFromSelector(_cmd)); 
    //录音完成后关闭释放资源 
    if ([recorder isRecording]) { 
         [recorder stop];
    } 
    // [recorder release];
    _audioRecorder = nil;
    // 录音的时候因为设置的音频会话是录音模式，所以录音完成后要把音频会话设置回听筒模式或者扬声器模式，根据切换器的值判断 
    AVAudioSession *audioSession = [AVAudioSession sharedInstance]; 
    
    // 扬声器模式 
	[audioSession setCategory:AVAudioSessionCategoryPlayback error:nil]; 
    // 听筒模式 
    // [audioSession setCategory:AVAudioSessionCategoryPlayAndRecord error:nil]; 

    [audioSession setActive:YES error:nil];
    kRecordAudioFile = NULL;
 }

//录音编码出错的回调 
- (void)audioRecorderEncodeErrorDidOccur:(AVAudioRecorder *)recorder error:(NSError *)error
{
     NSLog(@"%@",NSStringFromSelector(_cmd)); 
     // [recorder release];
} 

- (CMMotionManager *)motionManager
{
    if (!_motionManager) {
        _motionManager = [[CMMotionManager alloc] init];
    }
    return _motionManager;
}

- (void)startUpdateAccelerometer:(int)hz
{
    /* 设置采样的频率，单位是秒 */
    NSTimeInterval updateInterval = 1.0 / hz;
    
    /* 判断是否加速度传感器可用，如果可用则继续 */
    if ([self.motionManager isAccelerometerAvailable] == YES) {
		NSLog(@"Accelerometer avaliable");
		
		if ([self.motionManager isAccelerometerActive] == YES) {
			return;
		}
		
        /* 给采样频率赋值，单位是秒 */
        [self.motionManager setAccelerometerUpdateInterval:updateInterval];

        /* 加速度传感器开始采样，每次采样结果在block中处理 */
        [self.motionManager startAccelerometerUpdatesToQueue:[NSOperationQueue currentQueue] withHandler:^(CMAccelerometerData *accelerometerData, NSError *error) {
			CMAcceleration acceleration = accelerometerData.acceleration;
			// as if stop, it maybe call still!
            if (acceleration_callback) {
                acceleration_callback(acceleration.x, acceleration.y, acceleration.z);
            }
        }];
    }

}

- (void)startUpdateGyroscope:(int)hz
{
    /* 设置采样的频率，单位是秒 */
    NSTimeInterval updateInterval = 1.0 / hz;
    
    /* 判断是否陀螺仪可用，如果可用则继续 */
    if ([self.motionManager isGyroAvailable] == YES) {
		NSLog(@"Gyroscope avaliable");
		
		if ([self.motionManager isGyroActive] == YES) {
			return;
		}
		
        /* 给采样频率赋值，单位是秒 */
        [self.motionManager setGyroUpdateInterval:updateInterval];

        /* 陀螺仪开始采样，每次采样结果在block中处理 */
        [self.motionManager startGyroUpdatesToQueue:[NSOperationQueue currentQueue] withHandler:^(CMGyroData *gyroData, NSError *error) {
			CMRotationRate rotate = gyroData.rotationRate;
			// as if stop, it maybe call still!
            if (gyroscope_callback) {
                gyroscope_callback(rotate.x, rotate.y, rotate.z);
            }
        }];
    }

}

- (void)stopUpdateAccelerometer
{
    if ([self.motionManager isAccelerometerActive] == YES) {
        [self.motionManager stopAccelerometerUpdates];
        // [_motionManager release];
		_motionManager = nil;
    }

}

- (void)stopUpdateGyroscope
{
    if ([self.motionManager isGyroActive] == YES) {
        [self.motionManager stopGyroUpdates];
        // [_motionManager release];
		_motionManager = nil;
    }
}

@end

/* iPhone keyboard addition functions */
#if SDL_IPHONE_KEYBOARD

static SDL_uikitviewcontroller *
GetWindowViewController(SDL_Window * window)
{
    if (!window || !window->driverdata) {
        SDL_SetError("Invalid window");
        return nil;
    }

    SDL_WindowData *data = (__bridge SDL_WindowData *)window->driverdata;

    return data.viewcontroller;
}

SDL_bool
UIKit_HasScreenKeyboardSupport(_THIS)
{
    return SDL_TRUE;
}

void
UIKit_ShowScreenKeyboard(_THIS, SDL_Window *window)
{
    @autoreleasepool {
        SDL_uikitviewcontroller *vc = GetWindowViewController(window);
        [vc showKeyboard];
    }
}

void
UIKit_HideScreenKeyboard(_THIS, SDL_Window *window)
{
    @autoreleasepool {
        SDL_uikitviewcontroller *vc = GetWindowViewController(window);
        [vc hideKeyboard];
    }
}

SDL_bool
UIKit_IsScreenKeyboardShown(_THIS, SDL_Window *window)
{
    @autoreleasepool {
        SDL_uikitviewcontroller *vc = GetWindowViewController(window);
        if (vc != nil) {
            return vc.isKeyboardVisible;
        }
        return SDL_FALSE;
    }
}

void
UIKit_SetTextInputRect(_THIS, SDL_Rect *rect)
{
    if (!rect) {
        SDL_InvalidParamError("rect");
        return;
    }
    {
        rect->h = keyboardHeight2;
        return;
    }

    @autoreleasepool {
        SDL_uikitviewcontroller *vc = GetWindowViewController(SDL_GetFocusWindow());
        if (vc != nil) {
            vc.textInputRect = *rect;

            if (vc.keyboardVisible) {
                [vc updateKeyboard];
            }
        }
    }
}


#endif /* SDL_IPHONE_KEYBOARD */

void UIKit_StartRecordAudio(SDL_Window* window, const char* file)
{
	// 获取当前应用的AudioSession,每个应用都有自己唯一的声频会话 
	AVAudioSession* audioSession = [AVAudioSession sharedInstance]; 
	
	// 设置当前应用的声频会话为录音模式 
	NSError* error1;
	[audioSession setCategory:AVAudioSessionCategoryRecord error: &error1];
	// active audio session.
	[audioSession setActive:YES error: &error1];
    NSLog(@"record...");
    
    SDL_uikitviewcontroller *vc = GetWindowViewController(window);
    
    kRecordAudioFile = file;
    if (![vc.audioRecorder isRecording]) {
        [vc.audioRecorder record]; // 首次使用应用时如果调用record方法会询问用户是否允许使用麦克风
    }
	NSLog(@"record begin...");
}

void UIKit_StopRecordAudio(SDL_Window* window)
{
	if (!kRecordAudioFile) {
	 	return;
	}
	
	SDL_uikitviewcontroller *vc = GetWindowViewController(window);

	[vc.audioRecorder stop];
	NSLog(@"record stop...");
}

void UIKit_StartAccelerometer(SDL_Window* window, const int hz, fn_3axis_sensor_callback fn, SDL_bool gyroscope)
{
	SDL_uikitviewcontroller *vc = GetWindowViewController(window);
	if (gyroscope) {
		gyroscope_callback = fn;
		
		[vc startUpdateGyroscope:hz];
		NSLog(@"gyroscope begin...");
	} else {
		acceleration_callback = fn;
		
		[vc startUpdateAccelerometer:hz];
		NSLog(@"accelerometer begin...");
	}
}

void UIKit_StopAccelerometer(SDL_Window* window, SDL_bool gyroscope)
{
	SDL_uikitviewcontroller *vc = GetWindowViewController(window);
	if (gyroscope) {
		[vc stopUpdateGyroscope];
		NSLog(@"gyroscope stop...");
		
		gyroscope_callback = NULL;
	} else {
		[vc stopUpdateAccelerometer];
		NSLog(@"accelerometer stop...");
		
		acceleration_callback = NULL;
	}
}

void UIKit_ShowStatusBar(SDL_bool show)
{
	if (show) {
    	[[UIApplication sharedApplication] setStatusBarHidden:NO withAnimation:NO];
    } else {
    	[[UIApplication sharedApplication] setStatusBarHidden:YES withAnimation:NO];
    }
}

void UIKit_SetStatusBarStyle(SDL_bool light)
{
	if (light) {
    	[[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent];
    } else {
    	[[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault];
    }
}

#endif /* SDL_VIDEO_DRIVER_UIKIT */

/* vi: set ts=4 sw=4 expandtab: */
