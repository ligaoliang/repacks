from __future__ import with_statement
from contextlib import closing
from zipfile import ZipFile, ZIP_DEFLATED
import os

def zipdir(basedir, archivename, ignorelist=None):
    # remove last /
    if basedir.endswith(os.sep):
        basedir = basedir[:len(basedir) - len(os.sep)]
    assert os.path.isdir(basedir)
    with closing(ZipFile(archivename, 'w', ZIP_DEFLATED)) as z:
        for item in os.listdir(basedir):
            # ignore directory in the ignorelist
            if ignorelist is not None:
                if os.path.basename(item) in ignorelist:
                    continue

            item_name = os.path.join(basedir, item)
            if os.path.isdir(item_name):
                for root, dirs, files in os.walk(item_name):
                    for fn in files:
                        absfn = os.path.join(root, fn)
                        zfn = absfn[len(basedir) + len(os.sep):]  # relative path
                        z.write(absfn, zfn)
            else:
                zfn = item_name[len(basedir) + len(os.sep):] # relative path
                z.write(item_name, zfn)

if __name__ == '__main__':
    import sys
    basedir = sys.argv[1]
    archivename = sys.argv[2]
    zipdir(basedir, archivename, ['.git', '.gitignore'])
