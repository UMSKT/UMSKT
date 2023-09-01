# Universal MS Key Toolkit (UMSKT)

**Connect with us**

[![Zulip chat](https://img.shields.io/badge/zulip-join_chat-brightgreen.svg)](https://umskt.zulipchat.com)
[![libera.chat - #mspid](https://img.shields.io/badge/libera.chat-%23mspid-brightgreen)](https://web.libera.chat/gamja/?nick=Guest?#mspid)

**Build status**

[![C/C++ CI (Windows)](https://github.com/UMSKT/UMSKT/actions/workflows/windows.yml/badge.svg)](../../actions/workflows/windows.yml)

[![C/C++ CI (macOS)](https://github.com/UMSKT/UMSKT/actions/workflows/macos.yml/badge.svg)](../../actions/workflows/macos.yml)

[![C/C++ CI (Linux)](https://github.com/UMSKT/UMSKT/actions/workflows/linux.yml/badge.svg)](../../actions/workflows/linux.yml)

[![C/C++ CI (FreeBSD)](https://github.com/UMSKT/UMSKT/actions/workflows/freebsd.yml/badge.svg)](../../actions/workflows/dos-djgpp.yml)

[![C/C++ CI (DOS DJGPP)](https://github.com/UMSKT/UMSKT/actions/workflows/dos-djgpp.yml/badge.svg)](../../actions/workflows/freebsd.yml)

------

### Plan of Action / ToDo List

In light of the recent exponential interest in this project I've decided to put updates of this project here:

[Please see ticket #8 for more information](https://github.com/UMSKT/UMSKT/issues/8)

------

### **FAQ**

#### **What does it do?**

* This program is a tool for researching and experimenting with retro Microsoft product licensing, for products released before 2012.

#### **How does it work?**

* [Enderman's XPKeygen Readme explains everything in detail.](https://github.com/Endermanch/XPKeygen)

#### **How do I use it?**

* It all comes down to four simple steps:

------

### **Usage**
#### 1. Download the latest version of UMSKT

* *(GitHub account required)*
    * Download the latest experimental version using the Actions tab ([Windows](../../actions/workflows/windows.yml?query=branch%3Amaster+is%3Asuccess), [Linux](../../actions/workflows/linux.yml?query=branch%3Amaster+is%3Asuccess), [macOS](../../actions/workflows/macos.yml?query=branch%3Amaster+is%3Asuccess), [FreeBSD](../../actions/workflows/freebsd.yml?query=branch%3Amaster+is%3Asuccess), [DOS DJGPP](../../actions/workflows/dos-djgpp.yml?query=branch%3Amaster+is%3Asuccess))


* ~~*(GitHub account \*not\* required)*~~
    * ~~Download the latest release for your operating system and architecture from [the releases page](../../releases)~~
    * No official releases right now, use the other method to get the latest version.

* **Note:** Before continuing, please ensure you have the `umskt` executable extracted and on UNIX-like systems, have execution permissions (`chmod +x umskt`).

#### 2. Run `umskt` to generate a key, or add `--help` or `-h` to see more options.

#### 3. *(Activation step for `Retail` and `OEM` only)*
* After installation, you will be prompted to activate Windows.


* Select the **telephone activation** method, then, run `umskt -i <Installation ID>` using the `Installation ID` the activation Wizard provides for you

#### 4. Profit!


------


### Authors
The list of people who have helped to bring the XP generation to where it is now:
* z22
* MSKey
* diamondggg
* pottzman
* Endermanch
* Neo-Desktop
* WitherOrNot
* TheTank20
* InvoxiPlayGames
* brakmic
* techguy16

(the list will be updated to add more collaborators)

------

### **Development Requirements:**

* `build-essential`
  * `cmake`
  * `make`
  * `gcc`
  * `g++`
* `git`

#### Build Steps:

1. `git clone https://github.com/UMSKT/UMSKT`
2. `cd UMSKT/build`
3. `cmake ..`
4. `make`


-----

### **Known Ports**

| Language | Author    | Repo URL                                                  |
|----------|-----------|-----------------------------------------------------------|
| Rust     | Alex Page | [anpage/umskt-rs](https://github.com/anpage/umskt-rs)     |
| Python   | techguy16 | [techguy16/umsktpy](https://github.com/techguy16/umsktpy) |
