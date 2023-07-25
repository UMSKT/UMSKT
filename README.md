# Universal MS Key Toolkit (UMSKT)

**Connect with us**

[![Zulip chat](https://img.shields.io/badge/zulip-join_chat-brightgreen.svg)](https://umskt.zulipchat.com)
[![libera.chat - #mspid](https://img.shields.io/badge/libera.chat-%23mspid-brightgreen)](https://web.libera.chat/gamja/?nick=Guest?#mspid)

**Build status**
[![C/C++ CI (Linux)](https://github.com/UMSKT/UMSKT/actions/workflows/linux.yml/badge.svg)](../../actions/workflows/linux.yml)
[![C/C++ CI (Windows)](https://github.com/UMSKT/UMSKT/actions/workflows/windows.yml/badge.svg)](../../actions/workflows/windows.yml)
[![C/C++ CI (macOS)](https://github.com/UMSKT/UMSKT/actions/workflows/macos.yml/badge.svg)](../../actions/workflows/macos.yml)
[![C/C++ CI (DOS DJGPP)](https://github.com/UMSKT/UMSKT/actions/workflows/dos-djgpp.yml/badge.svg)](../../actions/workflows/freebsd.yml)
[![C/C++ CI (FreeBSD)](https://github.com/UMSKT/UMSKT/actions/workflows/freebsd.yml/badge.svg)](../../actions/workflows/dos-djgpp.yml)

Plan of Action / ToDo List

In light of the recent exponential interest in this project I've decided to put updates of this project here:

    Please see ticket #8 for more information

------

### **FAQ**

#### **What does it do?**

* This program is a tool for researching and experimenting with retro Microsoft product licensing, for products released before 2012.

#### **How does it work?**

* [Enderman's XPKeygen Readme explains everything in detail.](https://github.com/Endermanch/XPKeygen)

#### **How do I use it?**

* It all comes down to four simple steps:


### **Usage**
#### 1. Download the latest version of UMSKT

* *(GitHub account required)*
    * Download the latest experimental version using the Actions tab ([Windows](../../actions/workflows/windows.yml?query=branch%3Amaster), [Linux](../../actions/workflows/linux.yml?query=branch%3Amaster)), [macOS](../../actions/workflows/macos.yml?query=branch%3Amaster), [FreeBSD](../../actions/workflows/freebsd.yml?query=branch%3Amaster), [DOS DJGPP](../../actions/workflows/.yml?qudos-djgppery=branch%3Amaster))


* *(GitHub account \*not\* required)*
    * Download the latest release for your operating system and architecture from [the releases page](../../releases)


* **Note:** Before continuing, please ensure you have the `umskt` executable extracted.

#### 2. Run `umskt` to generate a key, or add `--help` to see more options.

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

(the list will be updated to add more collaborators)

------

### **Development Requirements:**

* `CMake, make, gcc` (`build-essential`)
* `git`

#### Build Steps:

1. `git clone`
2. `cd build/ && cmake ../ && make`


-----

### **Known Ports**

| Language | Author    | Repo URL                                              |
|----------|-----------|-------------------------------------------------------|
| Rust     | Alex Page | [anpage/umskt-rs](https://github.com/anpage/umskt-rs) |
| Python | techguy16 | No URL yet |
