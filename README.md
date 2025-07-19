<p align="center"><img src="https://avatars.githubusercontent.com/u/135211890?s=128&c=0" alt="umskt logo"/></p>

<h1 align="center"><b>U</b>niversal <b>MS</b> <b>K</b>ey <b>T</b>oolkit (UMSKT)</h1>

<p align="center">An open source toolkit designed to generate licence keys for MS products circa 1998 - 2006</p>
<hr />

**Connect with us**

[![Discord](https://img.shields.io/discord/1154155510887620729?label=discord&color=%235865F2)](https://discord.gg/PpBSpuphWM)
[![libera.chat - #mspid](https://img.shields.io/badge/libera.chat-%23mspid-brightgreen)](https://web.libera.chat/gamja/?nick=Guest?#mspid)
[![Zulip chat](https://img.shields.io/badge/zulip-join_chat-brightgreen.svg)](https://umskt.zulipchat.com)

----

| CI Build status |
| ------------ |
| [![C/C++ CI (Windows x86+64)](https://github.com/UMSKT/UMSKT/actions/workflows/windows-x86-x64.yml/badge.svg)](../../actions/workflows/windows-x86-x64.yml) |
| [![C/C++ CI (Windows ARM)](https://github.com/UMSKT/UMSKT/actions/workflows/windows-arm.yml/badge.svg)](../../actions/workflows/windows-arm.yml) |
| [![C/C++ CI (macOS)](https://github.com/UMSKT/UMSKT/actions/workflows/macos.yml/badge.svg)](../../actions/workflows/macos.yml) |
| [![C/C++ CI (Linux)](https://github.com/UMSKT/UMSKT/actions/workflows/linux.yml/badge.svg)](../../actions/workflows/linux.yml) |
| [![C/C++ CI (FreeBSD)](https://github.com/UMSKT/UMSKT/actions/workflows/freebsd.yml/badge.svg)](../../actions/workflows/freebsd.yml) |
| [![C/C++ CI (DOS DJGPP)](https://github.com/UMSKT/UMSKT/actions/workflows/dos-djgpp.yml/badge.svg)](../../actions/workflows/dos-djgpp.yml) |

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

### System Requirements
#### MS-DOS
* i386 processor or better
* MS-DOS 6.22 or later
* Any DOS-based version of Windows
* Windows NT 4.0 or later (via NTVDM)
#### Windows (x86/x64)
* i686 processor or better
* Windows XP or later

> [!WARNING]
> Processors barely meeting the minimum system requirements for XP may not work. Use the MS-DOS version via NTVDM in that case.

#### Windows (ARM32/64)
* Windows 11 21H2 or later

> [!NOTE]
> This is just all we can test. Try to get it to run on Windows RT, make an issue if it doesn't run and we'll try to fix it.
#### macOS
* Apple Silicon or x86_64 processor
* Latest version of macOS
#### Linux
* modern ARM, x86, or x86_64 processor
* Latest version of your Linux distro

------

### **Usage**
#### 1. Download the latest version of UMSKT

* *(GitHub account required)*
    * Download the latest experimental version using the Actions tab ([Windows](../../actions/workflows/windows.yml?query=branch%3Amaster+is%3Asuccess), [Linux](../../actions/workflows/linux.yml?query=branch%3Amaster+is%3Asuccess), [macOS](../../actions/workflows/macos.yml?query=branch%3Amaster+is%3Asuccess), [FreeBSD](../../actions/workflows/freebsd.yml?query=branch%3Amaster+is%3Asuccess), [DOS DJGPP](../../actions/workflows/dos-djgpp.yml?query=branch%3Amaster+is%3Asuccess))


* *(GitHub account \*not\* required)*
    * Download the latest release for your operating system and architecture from [the releases page](../../releases)

> [!IMPORTANT]
> Before continuing, please ensure you have the `umskt` executable extracted and on UNIX-like systems, have execution permissions (`chmod +x umskt`).

#### 2. Run `umskt` to generate a key, or add `--help` or `-h` to see more options.
> [!IMPORTANT]
> On macOS, like all unsigned executables, you'll need to hold Ctrl while right clicking and selecting Open to actually open it.

#### 3. *(Activation step for `Retail` and `OEM` only)*
* After installation, you will be prompted to activate Windows.


* Select the **telephone activation** method, then, run `umskt -i <Installation ID>` using the `Installation ID` the activation Wizard provides for you
   * If you're activating a non-Windows product, use `umskt -i <Installation ID> -m <Product>`, where `<Product>` is one of `OFFICEXP`, `OFFICE2K3`, `OFFICE2K7`, or `PLUSDME`
   * If activating Office 2003/2007, use `umskt -i <Installation ID> -m <Product> -p <Product ID>`

#### 4. Profit!


------


### Authors
The list of people who have helped to bring the XP generation to where it is now:
* z22
* MSKey
* diamondggg
* pottzman
* david4599
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
