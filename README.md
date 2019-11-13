# interceptor

A linux kernel module which hooks into the sys_call_table of the linux kernel. It replaces the sys_exit() function pointer just for print out the PID. at the end it calls the origin kernel function.

<br>
to compile it:

```
make

```


to load it:

```
sudo insmod interceptor.ko && sudo rmmod interceptor.ko
```
