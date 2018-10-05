# DbHashCracker
A tool which creates a hash table in mysql to crack md5 hashes fast

[![Build Status](https://travis-ci.org/0xFEEDC0DE64/DbHashCracker.svg?branch=master)](https://travis-ci.org/0xFEEDC0DE64/DbHashCracker) [![Codacy Badge](https://api.codacy.com/project/badge/Grade/906fba8525f54077a1450a0a638b69c6)](https://www.codacy.com/app/0xFEEDC0DE64/DbHashCracker?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=0xFEEDC0DE64/DbHashCracker&amp;utm_campaign=Badge_Grade)

Help output:

    ./hashcracker -h
    Usage: ./hashcracker [options]

    Options:
      -h, --help                       Displays this help.
      -v, --version                    Displays version information.
      -f, --filename <filename>        File containing the wordlist
      -n, --table-name <table>         Name of the table to be filled
      -d, --no-drop-table              Don't drop the (old) hash table at the
                                       beginning
      -c, --no-create-table            Don't create a table at the beginning
      -i, --no-index                   Don't add an index on hash column
      -p, --no-primary                 Don't add an primary key on value column
      -t, --threads <threads>          Thread count
      -b, --buffer-size <buffer-size>  Buffer size (per thread)
      -r, --no-replace-into            Use INSERT INTO instead of REPLACE INTO
      --no-delayed                     Dont add DELAYED keyword when inserting

Example run:

    $ ./hashcracker
    13:39:20.982 [D] filename = "wordlist.txt"
    13:39:20.983 [D] tableName = "Hashes"
    13:39:20.983 [D] noDropTable = false
    13:39:20.983 [D] noCreateTable = false
    13:39:20.983 [D] noIndex = false
    13:39:20.983 [D] noPrimary = false
    13:39:20.983 [D] threads = 8
    13:39:20.983 [D] bufferSize = 100
    13:39:20.983 [D] noReplace = false
    13:39:20.983 [D] noDelayed = false
    13:39:20.983 [D] reading wordlist...
    13:39:20.998 [D] connecting to database for "qt_sql_default_connection"
    13:39:21.938 [D] dropping old table...
    13:39:22.582 [D] creating table...
    13:39:23.777 [D] connecting to database for "thread0"
    13:39:23.777 [D] connecting to database for "thread1"
    13:39:23.777 [D] connecting to database for "thread2"
    13:39:23.777 [D] connecting to database for "thread3"
    13:39:23.777 [D] connecting to database for "thread4"
    13:39:23.777 [D] connecting to database for "thread5"
    13:39:23.777 [D] connecting to database for "thread6"
    13:39:23.777 [D] connecting to database for "thread7"
    13:39:24.829 [I] 0:00:01 | 0/4740047104 (0.00%) | NOW 0/sec 0:00:00 | AVG 0/sec 0:00:00
    13:39:25.878 [I] 0:00:02 | 300/4740047104 (0.00%) | NOW 300/sec 4388:55:56 | AVG 150/sec 8777:51:52
    13:39:26.928 [I] 0:00:03 | 600/4740047104 (0.00%) | NOW 300/sec 4388:55:55 | AVG 200/sec 6583:23:52
    13:39:27.978 [I] 0:00:04 | 1200/4740047104 (0.00%) | NOW 600/sec 2194:27:56 | AVG 300/sec 4388:55:53
    13:39:29.001 [I] 0:00:05 | 1600/4740047104 (0.00%) | NOW 400/sec 3291:41:53 | AVG 320/sec 4114:37:22
    13:39:30.001 [I] 0:00:06 | 2000/4740047104 (0.00%) | NOW 400/sec 3291:41:52 | AVG 333/sec 3953:59:29
    13:39:31.001 [I] 0:00:07 | 2700/4740047104 (0.00%) | NOW 700/sec 1880:58:12 | AVG 385/sec 3419:56:43
    13:39:32.001 [I] 0:00:08 | 3200/4740047104 (0.00%) | NOW 500/sec 2633:21:27 | AVG 400/sec 3291:41:49
    ^C

## Building from source
This project can only be built as part of the project structure [DbSoftware](https://github.com/0xFEEDC0DE64/DbSoftware)

```Shell
git clone https://github.com/0xFEEDC0DE64/DbSoftware.git
cd DbSoftware
git submodule update --init --recursive DbHashCracker
cd ..
mkdir build_DbSoftware
cd build_DbSoftware
qmake CONFIG+=ccache ../DbSoftware
make -j$(nproc) sub-DbHashCracker
make sub-DbHashCracker-install_subtargets
./bin/hashcracker
```
