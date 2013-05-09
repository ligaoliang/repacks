import os

DEBUG=True

ROOT = os.path.dirname(os.path.abspath(__file__))
path = lambda *a: os.path.join(ROOT, *a)

# addon config files directory
CONF_DIR = path('conf')

# keys directory
KEYS_DIR = path('keys')

# Default XPI name
DEF_XPI_NAME = 'latest.xpi'

# XPIs output directory
# XPI will be named DEF_XPI_NAME, and is for other packer.
OUT_DIR = os.path.abspath(path('..', 'addons', 'official'))

# Deploying directory
# XPI will be named as branch name plus version number, it is for deploying
DEPLOY_DIR = path('deploy')

# Log file path to record deploy history
PACK_LOG_DIR = path('deploy', 'logs')

# repository directory to pull addon sourcecodes
REPO_DIR = path('repos')

# working dir
WORK_DIR = path('tmp')

# import local settings
try:
    from settings_local import *
except:
    print 'Exception occurs when importing settings_local'
