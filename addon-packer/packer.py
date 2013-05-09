# !/user/bin/env python
__version__ = '0.1'

from settings import *
from configobj import ConfigObj
from subprocess import call, Popen, PIPE, CalledProcessError
from util import zipdir
from colorama import init, Fore, Back, Style

import os, sys, shutil, optparse

# Init Colorama
init()

# from https://gist.github.com/1027906
def check_output(*popenargs, **kwargs):
    r"""Run command with arguments and return its output as a byte string.

Backported from Python 2.7 as it's implemented as pure python on stdlib.

>>> check_output(['/usr/bin/python', '--version'])
Python 2.6.2
"""
    process = Popen(stdout=PIPE, *popenargs, **kwargs)
    output, unused_err = process.communicate()
    retcode = process.poll()
    if retcode:
        cmd = kwargs.get("args")
        if cmd is None:
            cmd = popenargs[0]
        error = CalledProcessError(retcode, cmd)
        error.output = output
        raise error
    return output

def debug(s):
    if DEBUG:
        print(Fore.GREEN + s + Fore.RESET)

def log(s):
    print(s)

def error(s):
    print(Fore.RED + s + Fore.RESET)

def shell_command(cmd, cwd=None):
    # Shell command output gets dumped immediately to stdout, whereas
    # print statements get buffered unless we flush them explicitly.
    sys.stdout.flush()

    debug('Execute command: \n%s' % cmd)

    if cwd is None:
        ret = call(cmd, shell=True)
    else:
        ret = call(cmd, cwd=cwd, shell=True)

    if ret != 0:
        ret_real = (ret & 0xFF00) >> 8
        error("Error, shellCommand had non-zero exit status: %d" % ret_real)
        error("command was: %s" % cmd)
        raise Exception('Command had non-zero exit status.')

    return True

def _get_repo_path(name):
    return path(REPO_DIR, name)

'''
Prepare working dir, if does not exist, create it.
Working dir must be created.
'''
def prepare_working_dir():
    if not os.path.exists(WORK_DIR):
        log('Create working dir .. ')
        os.makedirs(WORK_DIR, 0777)
    else:
        error('Working dir already exists!')
        sys.exit(1)

def clean_working_dir():
    log('Clean and remove working dir .. ')
    if os.path.exists(WORK_DIR):
        shutil.rmtree(WORK_DIR)

'''
Checkout branch as an temp branch
'''
def checkout_branch(config_obj):
    _repo_path = _get_repo_path(config_obj['name'])
    if not os.path.exists(_repo_path):
        error('Repo does not exist: %s' % _repo_path)
        raise Exception('Repo does not exists.')

    branch = config_obj['branch']
    log('Checkout branch origin/%s' % branch)
    shell_command('git checkout origin/%s' % branch, cwd=_repo_path)

def get_install_info(update_rdf_file):
    if not os.path.exists(update_rdf_file):
        raise Exception('File not found: %s' % update_rdf_file)

    return parse_install_info(''.join(open(update_rdf_file).readlines()))

def parse_install_info(rdf_string):
    from lxml import etree
    _tree = etree.fromstring(rdf_string)
    nsmap = {
        'em': 'http://www.mozilla.org/2004/em-rdf#',
        'RDF': 'http://www.w3.org/1999/02/22-rdf-syntax-ns#',
    }
    _description = _tree.find('RDF:Description', nsmap)
    # If addon is from AMO, updateURL will be empty
    _update_url = _description.findtext('em:updateURL', None, nsmap)

    _version = _description.findtext('em:version', None, nsmap)
    if _version is None:
        raise UserWarning('No em:version in install.rdf')

    return {
        'version': _version,
        'updateURL': _update_url,
    }

'''
Pack addon by given config object
'''
def pack_xpi(config_obj, deploy=False, check_history=False):
    id       = config_obj['id']
    name     = config_obj['name']
    branch   = config_obj['branch']
    base_dir = config_obj['basedir']
    signkey  = config_obj['signkey']

    _repo_path = _get_repo_path(name)
    if not os.path.exists(_repo_path):
        error('Repo does not exist: %s' % _repo_path)
        raise Exception('Repo does not exists.')

    commit_hash = check_output('git log -1 --pretty=format:%H', cwd=_repo_path)

    # check packing history, if the version of the branch have been handled, skip it.
    # Config obj contain branch deploy history
    if not os.path.exists(PACK_LOG_DIR):
        os.makedirs(PACK_LOG_DIR)
    _pack_his_file = os.path.join(PACK_LOG_DIR, name)
    _pack_history = ConfigObj(_pack_his_file)
    if check_history is True:
        if _pack_history.has_key(branch) and _pack_history[branch] == commit_hash:
            log((Fore.BLUE + 'the branch %s of %s has been handled, skip it.' + Fore.RESET) % (branch,name))
            return

    _base_dir = os.path.join(_repo_path, base_dir)
    log('Extension base dir: %s' % _base_dir)

    # Check if install.rdf exists
    _install_rdf_file = os.path.join(_base_dir, 'install.rdf')
    if not os.path.exists(_install_rdf_file):
        error('Install.rdf does not exists: %s' % _install_rdf_file)
        raise Exception('Install.rdf does not exists.')

    # zip files into an XPI file
    _xpi_dir = os.path.join(OUT_DIR, id, branch)
    if not os.path.exists(_xpi_dir):
        log('Create dir for XPI file ...')
        os.makedirs(_xpi_dir)

    _xpi_file = os.path.join(_xpi_dir, DEF_XPI_NAME)
    log('Zip dir into XPI file: %s' % _xpi_file)
    zipdir(_base_dir, _xpi_file, ['.git', '.gitignore', 'application.ini', 'documents', 'README'])

    # record deploy history
    _pack_history[branch] = commit_hash
    _pack_history.write()

def _parse_target_dir(install_info, config_obj):
    _update_url = install_info['updateURL']
    branch = config_obj['branch']
    name = config_obj['name']

    _url_prefix = 'http://'
    # composite deploy path from update url
    # No update url is defined, may be it an addon on the AMO
    if _update_url is not None and branch not in config_obj['sign_ignore']:
        _xpi_dir = DEPLOY_DIR
        for d in os.path.dirname(_update_url)[len(_url_prefix):].split('/'):
            _xpi_dir = os.path.join(_xpi_dir, d)
    else:
        _xpi_dir = os.path.join(DEPLOY_DIR, name, '' if branch == 'master' else branch)

    _xpi_base_name = ''.join([name, '-', install_info['version'], '.xpi'])

    return [_xpi_dir, _xpi_base_name]

'''
Sign an given xpi file with config
'''
def sign_xpi_file(xpi_file, config_obj, output_dir=None):
    if output_dir is not None:
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)
    else:
        # use the same dir as the base dir of xpi_file
        output_dir = os.path.dirname(xpi_file)

    _sign_xpi(xpi_file, config_obj['signkey'], '.', os.path.join(output_dir, 'update.rdf'))

'''
xpi_file:
  XPI file path
signkey:
  sign key name, like cehomepage.pem
update_link:
  like: http://g-fox.cn/chinaedition/livemargins/livemargins-5.1.xpi
update_rdf_file:
  output file path
'''
def _sign_xpi(xpi_file, signkey, update_link, update_rdf_file):
    import sulu
    _keyfile = os.path.join(KEYS_DIR, signkey)
    _override_file = path('sulu', 'override.txt')

    if not os.path.exists(_keyfile):
        raise Exception("Signkey file does not exist: %s" % _keyfile)

    if os.getenv('SIGNPASS') is None:
        raise Exception('$SIGNPASS is None.')

    get_passphrase = sulu.pass_phrase_cb('$SIGNPASS')
    get_max_version = sulu.max_version_cb(_override_file)
    sulu.sign_update_rdf([(xpi_file, update_link, get_max_version)], _keyfile, update_rdf_file, get_passphrase)

    log('Generating update.rdf done')

def _wrap_config_obj(config, branch=None):
    _config_obj = {
        'id':       config['id'],
        'name':     config['name'],
        'git':      config['git'],
        'branches': config['branches'],
        'branch':   branch if branch is not None else config['branches'],
        'basedir':  config['basedir'],
        'signkey':  config.has_key('signkey') and config['signkey'],
        'jetpack':  config.has_key('jetpack') and config['jetpack'] == 'True',
        'sign_ignore': []
    }

    if config.has_key('sign_ignore') and config['sign_ignore'] is not None:
        if type(config['sign_ignore']) is list:
            _config_obj['sign_ignore'] = config['sign_ignore']
        else:
            _config_obj['sign_ignore'] = [config['sign_ignore']]

    return _config_obj

def get_config_obj(conf_name):
    _config = ConfigObj(path(CONF_DIR, conf_name))
    return _wrap_config_obj(_config)

'''
Get config object array based on the branches
'''
def get_config_obj_2(conf_name):
    _config = ConfigObj(path(CONF_DIR, conf_name))

    _branches = _config['branches']
    if type(_branches) is list:
        return [_wrap_config_obj(_config, branch) for branch in _branches]
    else:
        return [_wrap_config_obj(_config)]


'''
Walk all config files ignore the branches
'''
def walk_all_conf(callback):
    for conf_name in os.listdir(CONF_DIR):
        # Only handle the config file ends with .conf
        if conf_name.endswith('.conf'):
            callback(get_config_obj(conf_name))

'''
Walk all config files, if contains more than on branch, split it into single object
'''
def walk_all_conf_2(callback):
    for conf_name in os.listdir(CONF_DIR):
        # Only handle the config file ends with .conf
        if conf_name.endswith('.conf'):
            for cb in get_config_obj_2(conf_name):
                callback(cb)

'''
Clone or update repository by given config obj
'''
def clone_or_upd_repo(config_obj):
    if not os.path.exists(REPO_DIR):
        os.makedirs(REPO_DIR)

    if not config_obj['jetpack']:
        # update repository first
        _repo_path = _get_repo_path(config_obj['name'])
        if not os.path.exists(_repo_path):
            log('Repository does not exists, clone: %s' % config_obj['git'])
            shell_command('git clone %s "%s"' % (config_obj['git'],_repo_path))
            log('Clone git repository done')
        else:
            log('Update repository: %s' % config_obj['name'])
            shell_command('git fetch', cwd=_repo_path)
            log('Update repository done.')
    else:
        log('Skip to update repository for Jetpack addon')

'''
Clone or update all repositories
'''
def update_all_repos():
    walk_all_conf(clone_or_upd_repo)

'''
Pack all repositories
'''
def pack_all_xpis(deploy=False, check_history=False):
    def _pack_xpi(config_obj):
        # TODO pack Jetpack addon
        # Only pack the normal extension excluding the Jetpack addon
        if not config_obj['jetpack']:
            checkout_branch(config_obj)
            if deploy is False:
                pack_xpi(config_obj, check_history=check_history)
            else:
                deploy_xpi(config_obj)
        else:
            log("Skip to pack Jetpack addon.")
    walk_all_conf_2(_pack_xpi)

'''
Deploy xpi by given config object
'''
def deploy_xpi(config_obj):
    id = config_obj['id']
    branch = config_obj['branch']
    name = config_obj['name']

    _xpi_dir = os.path.join(OUT_DIR, id, branch)
    _xpi_file = os.path.join(_xpi_dir, DEF_XPI_NAME)

    if not os.path.exists(_xpi_file):
        print _xpi_file + Fore.RED + ' does not exist' + Fore.RESET
        return
    else:
        print 'deploy and sign: ' + Fore.BLUE + _xpi_file + Fore.RESET

    import sulu
    _install_info = parse_install_info(sulu.get_install_string(_xpi_file))

    if _install_info['updateURL'] is None:
        print Fore.RED + 'no update link' + Fore.RESET
        return

    [_xpi_dir, _xpi_base_name] = _parse_target_dir(_install_info, config_obj)
    _target_xpi = os.path.join(_xpi_dir, _xpi_base_name)

    if not os.path.exists(_xpi_dir):
        log('create dir for xpi file ...')
        os.makedirs(_xpi_dir)

    # Copy file to the deploy dir
    shutil.copyfile(_xpi_file, _target_xpi)

    if branch not in config_obj['sign_ignore']:
        # Sign xpi file
        sign_xpi_file(_target_xpi, config_obj)
    else:
        print Fore.BLUE + 'Ignoring' + Fore.RESET + ' sign update.rdf'

'''
Copy packed XPI files into deploy directory, and sign update.rdf
'''
def deploy_packed_addons():
    def _deploy(config_obj):
        deploy_xpi(config_obj)

    walk_all_conf_2(_deploy)

def parse_opts():
    parser = optparse.OptionParser()
    parser.add_option('-u', '--update', action='store_true',
                      dest='update', default=False,
                      help='update repository')
    parser.add_option('-p', '--pack', action='store_true',
                      dest='pack', default=False,
                      help='pack XPI')
    parser.add_option('-c', '--check-history', action='store_true',
                      dest='check_history', default=False,
                      help='Check history when deploying XPI')
    parser.add_option('-r', '--remove-history', action='store_true',
                      dest='remove_history', default=False,
                      help='Remove deploy history')
    parser.add_option('-S', '--sign-addon', action='store',
                      dest='sign_addon', help='Sign given addon')
    parser.add_option('-s', '--sign-deploy-addons', action='store_true',
                      dest='sign_deploy_addons', default=False,
                      help='Sign and deploy the addons in the $OUT_DIR')
    parser.add_option('-o', '--output', action='store',
                      dest='output', help='Output dir, if none, use the same base dir as "-s"')

    return parser.parse_args()

def main():
    options, args = parse_opts()

    if not options.update and not options.pack:
        if not options.remove_history:
            if not options.check_history and options.sign_addon is None:
                if not options.sign_deploy_addons:
                    print('USAGE: packer.py -[updcrsS] [config_name]')
                    return

    # Sign an single xpi file with given config name
    if options.sign_addon is not None:
        if len(args) != 1:
            print 'Please give the config name'
            return

        _config_name = args[0]
        if not _config_name.endswith('.conf'):
            _config_name += '.conf'

        _config_obj = get_config_obj(_config_name)

        sign_xpi_file(options.sign_addon, _config_obj, output_dir=options.output)
        return

    if len(args) > 0:
        for config_name in args:
            if options.remove_history is True:
                _pack_his_file = os.path.join(PACK_LOG_DIR, config_name)
                if os.path.exists(_pack_his_file):
                    os.remove(_pack_his_file)

            if not config_name.endswith('.conf'):
                config_name = config_name + '.conf'

            # update repository on once
            _config_obj = get_config_obj(config_name)
            if options.update:
                clone_or_upd_repo(_config_obj)

            for co in get_config_obj_2(config_name):
                if options.pack:
                    checkout_branch(co)
                    pack_xpi(co, check_history=options.check_history)
                if options.sign_deploy_addons:
                    deploy_xpi(co)
    else:
        if options.remove_history:
            log('Clean deploy history.')
            if os.path.exists(PACK_LOG_DIR):
                shutil.rmtree(PACK_LOG_DIR)
        if options.update:
            update_all_repos()
        if options.pack:
            pack_all_xpis(check_history=options.check_history)
        # Sign and deploy the addons in the $OUT_DIR
        if options.sign_deploy_addons:
            deploy_packed_addons()


if __name__ == '__main__':
    main()
