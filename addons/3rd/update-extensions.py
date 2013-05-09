#!/usr/bin/env python

import hashlib, os, re, sys
from os import path
from optparse import OptionParser
import urllib

#########################################################################
def get_xpi_hash(xpi_file):
    xpi = open(xpi_file, 'rb')
    xpi_string = xpi.read()
    xpi.close()

    return '%s:%s' % ('sha256', hashlib.sha256(xpi_string).hexdigest())

#########################################################################
def rmdirRecursive(dir):
    """This is a replacement for shutil.rmtree that works better under
    windows. Thanks to Bear at the OSAF for the code.
    (Borrowed from buildbot.slave.commands)"""
    if not os.path.exists(dir):
        # This handles broken links
        if os.path.islink(dir):
            os.remove(dir)
        return

    if os.path.islink(dir):
        os.remove(dir)
        return

    # Verify the directory is read/write/execute for the current user
    os.chmod(dir, 0700)

    for name in os.listdir(dir):
        full_name = os.path.join(dir, name)
        # on Windows, if we don't have write permission we can't remove
        # the file/directory either, so turn that on
        if os.name == 'nt':
            if not os.access(full_name, os.W_OK):
                # I think this is now redundant, but I don't have an NT
                # machine to test on, so I'm going to leave it in place
                # -warner
                os.chmod(full_name, 0600)

        if os.path.isdir(full_name):
            rmdirRecursive(full_name)
        else:
            # Don't try to chmod links
            if not os.path.islink(full_name):
                os.chmod(full_name, 0700)
            os.remove(full_name)
    os.rmdir(dir)

#########################################################################
def printSeparator():
    print "##################################################"

#########################################################################
def mkdir(dir, mode=0755):
    if not os.path.exists(dir):
        return os.makedirs(dir, mode)
    return True

#########################################################################
def isValidFile(file_path):
    # app.tag is too small in size for the original test
    return True

#########################################################################
def retrieveFile(url, file_path):
  failedDownload = False
  try:
    urllib.urlretrieve(url.replace(' ','%20'), file_path)
  except:
    print "exception: n  %s, n  %s, n  %s n  when downloading %s" % \
          (sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2], url)
    failedDownload = True

  if os.path.exists(file_path):
    if not isValidFile(file_path):
      failedDownload = True
      try:
        os.remove(file_path)
      except:
        print "exception: n  %s, n  %s, n  %s n  when trying to remove file %s" %\
              (sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2], file_path)
  else:
    failedDownload = True

  if failedDownload:
    return False
  else:
    return True

#########################################################################
def getSingleFileFromAMO(src, file):
    idx = src.find('/')
    id = src[:idx] if idx > -1 else src
    url = src if src.startswith('http') else 'https://addons.mozilla.org/firefox/downloads/latest/%s/addon-%s-latest.xpi' % (src, id)
    print "Downloading: %s" % url
    return retrieveFile(url, file)

#########################################################################
if __name__ == '__main__':
    error = False

    files = open('extensions.lst').read().split('\n')
    hashes = open('hashes.txt', 'w')
    for file in files:
        src, target = file.split(' ')
        if not getSingleFileFromAMO(src, target):
            print "Error: Unable to retrieve %s" % target
            sys.exit(1)
        hashes.write('%s %s\n' % (target, get_xpi_hash(target)))
    hashes.close()
