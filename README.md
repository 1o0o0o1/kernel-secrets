# Kenel secrets

The module is a simple repository of arbitrary user secrets.
When loading the module, the /proc/secret directory is created containing the set_index, read, write, delete files, which we will call interfaces. With the help of these interfaces, the user can save some data, read it and delete it. For this, each secret is assigned a special numeric identifier.

## Usage

First of all, you need to set the index value using the set_index interface.
This can be done directly, for example through the echo command, or use the provided utility to work with the module.

`set_index` -- accepts and sets an index for storing data
`read` -- allows you to read data with the previously set index.
`write` -- allows you to write data with the previously set index.
`delete` -- allows you to delete data with a previously set index.

## Examples:

At the moment, all information is output to the dmesg kernel log. You should check the operation status in dmesg

### Manual method:
```shell
# set index 0
echo "0" > /proc/secret/s
# set index 0
echo "0" > /proc/secret/set_index

# write some data to secret whith index 0
echo "some data" > /proc/secret/write

# read secret whith index 0
cat /proc/secret/read # "some data"

#delete secret whith index 0
cat /proc/secret/delete
```
### Using the Python utility:
```shell
python3 ksecrets.py
```

## Installation

A Makefile is used for the build:
! Make sure you have the necessary permissions!

1. Go to the project directory and run `make`;
2. After the build, load the module with the command `insmod secret.ko`;
3. Run the utility `ksecrets.py ` or use the manual method described earlier.