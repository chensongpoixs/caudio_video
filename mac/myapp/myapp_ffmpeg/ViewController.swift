//
//  ViewController.swift
//  myapp_ffmpeg
//
//  Created by chensong on 2022/3/22.
//

import Cocoa

class ViewController: NSViewController {

    override func viewDidLoad() {
        super.viewDidLoad()

        // Do any additional setup after loading the view.
        self.view.setFrameSize(NSSize(width: 640, height: 320));
        
        let btn = NSButton.init(title: "Button", target: nil, action: nil);
        
        btn.title = "hello world !!!";
        
        btn.frame = NSRect(x:640/2-40, y:240/2-15, width: 80, height: 30);
        
        btn.bezelStyle = .rounded;
        btn.setButtonType(.pushOnPushOff);
        
        /** 设置回callback**/
        
        btn.target = self;
        btn.action = #selector(myfunc);
        
        self.view.addSubview(btn);
    }

    @objc
    func myfunc()
    {
//        print("callback!!!");
        swift_call_c();
    }
    
    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }


}

