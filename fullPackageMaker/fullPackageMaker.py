#!/usr/bin/python
from __future__ import print_function
from xml.dom import minidom
from subprocess import Popen
from subprocess import call
import os, sys, codecs, json
from datetime import date
from urlparse import urlparse
from configparser import ConfigParser
from jinja2 import Environment, FileSystemLoader

from colorama import init, Fore, Back, Style

def mkdir(dir, mode=0755):
	if not os.path.exists(dir):
		return os.makedirs(dir, mode)
	return True

def get_path(url):
	return urlparse(url).path

def shell_command(cmd):
	try:
		retcode = call(cmd,shell=True)
		if retcode == 0:
			print ('\t%sOK%s %s' % (Fore.GREEN, Fore.RESET, cmd))
		else:
			print ('\t%sError %s%s' % (Fore.RED, cmd, Fore.RESET))
			sys.exit(retcode)
	except OSError, e:
		print >>sys.stderr, "	run Execution failed:", e
		sys.exit(-1)

class FullPackageMaker:
	""""FullPackageMaker is a tool for making full packages"""

	def  __init__(self,channel,version):
		print ('%sFull Installer (channel: %s)%s' % (Fore.CYAN, channel, Fore.RESET))
		self.channel = channel
		self.appVer = ''
		self.packageUrl = ''
		self.resUrl = ''
		self.extUrl = ''
		self.picList = ['Default.bmp','other.bmp']
		self.addonList = {
			'hidden': [],
			'default': [],
			'other': []
		}
		self.rebuildFlag = '0'
		self.scriptDir = ''
		self.template =''
		self.bundle = '1'
		self.version = version
		self.locale = 'zh-CN'

	def ParseXML(self):
		"""Parse XMLs which contain all the configeration info."""
		print ('%sParse config.xml%s' % (Fore.CYAN, Fore.RESET))
		xmldoc = minidom.parse('config/' + self.channel + '/config.xml')
		appVer = xmldoc.firstChild.getElementsByTagName('AppVersion')[0]
		for subNode in appVer.childNodes:
			if subNode.nodeType == appVer.TEXT_NODE:
				self.appVer = subNode.data.replace('@VERSION@', self.version)
		packageUrl = xmldoc.firstChild.getElementsByTagName('packageUrl')[0]
		for subNode in packageUrl.childNodes:
			if subNode.nodeType == packageUrl.TEXT_NODE:
				self.packageUrl = subNode.data.replace('@CHINA_CHANNEL_PREFIX@', 'esr/firefox' if self.version.endswith('esr') else 'firefox').replace('@VERSION_PREFIX@', '.'.join(self.version.split('.')[:2])).replace('@MONDD@', date.strftime(date.today(), '%b%d'))
		resUrl = xmldoc.firstChild.getElementsByTagName('resUrl')[0]
		for subNode in resUrl.childNodes:
			if subNode.nodeType == resUrl.TEXT_NODE:
				self.resUrl = subNode.data.replace('@VERSION_PREFIX@', '.'.join(self.version.split('.')[:2]))
		extUrl = xmldoc.firstChild.getElementsByTagName('extUrl')[0]
		for subNode in extUrl.childNodes:
			if subNode.nodeType == extUrl.TEXT_NODE:
				self.extUrl = subNode.data.replace('@VERSION_PREFIX@', '.'.join(self.version.split('.')[:2]))
		setup = xmldoc.firstChild.getElementsByTagName('setup')[0]
		self.rebuildFlag = setup.getAttribute('rebuild')
		scriptDir = setup.getElementsByTagName('scriptDir')[0]
		for subNode in scriptDir.childNodes:
			if subNode.nodeType == scriptDir.TEXT_NODE:
				self.scriptDir = subNode.data
		template = xmldoc.firstChild.getElementsByTagName('template')[0]
		for subNode in template.childNodes:
			if subNode.nodeType == template.TEXT_NODE:
				self.template = subNode.data
		locale = xmldoc.firstChild.getElementsByTagName('locale')[0]
		for subNode in locale.childNodes:
			if subNode.nodeType == locale.TEXT_NODE:
				self.locale = subNode.data
		distribution = xmldoc.firstChild.getElementsByTagName('distribution')[0]
		self.bundle = distribution.getAttribute('bundle')

		addonCategory = xmldoc.firstChild.getElementsByTagName('Addons')[0]
		hidden = addonCategory.getElementsByTagName('hidden')[0]
		addons = hidden.getElementsByTagName("addon")
		for addon in addons:
			id = addon.getElementsByTagName("id")[0]
			for id in id.childNodes:
				if id.nodeType in (id.TEXT_NODE,id.CDATA_SECTION_NODE):
					self.addonList['hidden'].append({'id': id.data})
			branch = addon.getElementsByTagName("branch")[0]
			for branch in branch.childNodes:
				if branch.nodeType in (branch.TEXT_NODE,branch.CDATA_SECTION_NODE):
					self.addonList['hidden'][-1]['branch'] = branch.data
		default = addonCategory.getElementsByTagName('default')[0]
		addons = default.getElementsByTagName("addon")
		for addon in addons:
			id = addon.getElementsByTagName("id")[0]
			for id in id.childNodes:
				if id.nodeType in (id.TEXT_NODE,id.CDATA_SECTION_NODE):
					self.addonList['default'].append({'id': id.data})
			branch = addon.getElementsByTagName("branch")[0]
			for branch in branch.childNodes:
				if branch.nodeType in (branch.TEXT_NODE,branch.CDATA_SECTION_NODE):
					self.addonList['default'][-1]['branch'] = branch.data
		other = addonCategory.getElementsByTagName('other')[0]
		addons = other.getElementsByTagName("addon")
		for addon in addons:
			id = addon.getElementsByTagName("id")[0]
			for id in id.childNodes:
				if id.nodeType in (id.TEXT_NODE,id.CDATA_SECTION_NODE):
					self.addonList['other'].append({'id': id.data})
			branch = addon.getElementsByTagName("branch")[0]
			for branch in branch.childNodes:
				if branch.nodeType in (branch.TEXT_NODE,branch.CDATA_SECTION_NODE):
					self.addonList['other'][-1]['branch'] = branch.data

	def MakeDistributionFolder(self):
		print ('%sPrepare distribution folder%s' % (Fore.CYAN, Fore.RESET))
		os.makedirs('build/distribution/extensions/')
		shell_command('cp -p config/%s/distribution.ini build/distribution/' % self.channel)
		if os.path.exists('../common/distribution/common/searchplugins/'):
			shell_command('cp -rp ../common/distribution/common/searchplugins/ build/distribution/')
		cp_dist = ConfigParser()
		cp_dist.optionxform = str
		cp_dist.read(['../common/distribution/common/distribution.ini', 'build/distribution/distribution.ini'], 'utf-8')
		cp_dist.set('Global', 'version', '%d.%d' % date.timetuple(date.today())[:2])
		cp_dist.set('Preferences', 'app.taiwanedition.channel', cp_dist.get('Preferences', 'app.taiwanedition.channel').replace('@CHINA_CHANNEL@', 'esr.firefox.com.cn' if self.version.endswith('esr') else 'firefox.com.cn'))
		cp_dist.set('LocalizablePreferences-zh-TW', 'startup.homepage_welcome_url', cp_dist.get('LocalizablePreferences-zh-TW', 'startup.homepage_welcome_url').replace('@VERSION_PREFIX@', '.'.join(self.version.split('.')[:2])))
		fw = codecs.open('build/distribution/distribution.ini', 'wb', 'utf-8')
		cp_dist.write(fw, False)
		fw.close()
		cp_myext = ConfigParser()
		if self.bundle == '0':
			for addon in self.addonList['hidden']:
				if os.path.exists('../addons/official/%(id)s/%(branch)s/latest.xpi' % addon):
					filePath = '../addons/official/%(id)s/%(branch)s/latest.xpi' % addon
				elif os.path.exists('../addons/3rd/%(id)s/latest.xpi' % addon):
					filePath = '../addons/3rd/%(id)s/latest.xpi' % addon
				else:
					print('%sExtension %s not exist%s' % (Fore.RED, addon['id'], Fore.RESET))
					sys.exit(0)
				cmd = '7z.exe x %s -obuild/distribution/extensions/%s >> null' % (filePath, addon['id'])
				shell_command(cmd)
			for addon in self.addonList['default']:
				if os.path.exists('../addons/official/%(id)s/%(branch)s/latest.xpi' % addon):
					filePath = '../addons/official/%(id)s/%(branch)s/latest.xpi' % addon
				elif os.path.exists('../addons/3rd/%(id)s/latest.xpi' % addon):
					filePath = '../addons/3rd/%(id)s/latest.xpi' % addon
				else:
					print('%sExtension %s not exist%s' % (Fore.RED, addon['id'], Fore.RESET))
					sys.exit(0)
				cmd = '7z.exe x %s -obuild/distribution/extensions/%s >> null' % (filePath, addon['id'])
				shell_command(cmd)
			return
		shell_command('cp -rp ../common/distribution/common/bundles/ build/distribution/')
		ver_prefix = '.'.join(self.version.split('.')[:2])
		#if os.path.exists('../common/distribution/%s/bundles/' % ver_prefix):
		#	shell_command('cp -rp ../common/distribution/%s/bundles/ build/distribution/' % ver_prefix)
		os.makedirs('build/distribution/myextensions/')
		os.makedirs('build/myextensions/')
		shell_command('touch build/distribution/myextensions/config.ini')
		index = 0

		extension_xml = None
		if self.bundle == '1':
			os.makedirs('build/res')
			extension_xml = codecs.open('build/res/extensions.xml', 'w', 'utf-8')
			extension_data_json = open('../common/res/%s/data.json' % self.locale)
			extension_data = json.load(extension_data_json)
			extension_data_json.close()
			jinja2_context = {
				'hidden': [],
				'default': [],
				'other': []
			}

		for addon in self.addonList['hidden']:
			if os.path.exists('../addons/official/%(id)s/%(branch)s/latest.xpi' % addon):
				filePath = '../addons/official/%(id)s/%(branch)s/latest.xpi' % addon
			elif os.path.exists('../addons/3rd/%(id)s/latest.xpi' % addon):
				filePath = '../addons/3rd/%(id)s/latest.xpi' % addon
			else:
				print('%sExtension %s not exist%s' % (Fore.RED, addon['id'], Fore.RESET))
				sys.exit(0)
			cmd = 'cp -p %s build/distribution/myextensions/%s.xpi' % (filePath, addon['id'])
			shell_command(cmd)
			section = 'Extension%d' % index
			cp_myext.add_section(section)
			cp_myext.set(section, 'id', addon['id'])
			cp_myext.set(section, 'file', '%s.xpi' % addon['id'])
			cp_myext.set(section, 'os', 'all')
			index += 1
			if extension_xml:
				extension_data[addon['id']]['file'] = addon['id']
				jinja2_context['hidden'].append(extension_data[addon['id']])

		for addon in self.addonList['default']:
			if os.path.exists('../addons/official/%(id)s/%(branch)s/latest.xpi' % addon):
				filePath = '../addons/official/%(id)s/%(branch)s/latest.xpi' % addon
			elif os.path.exists('../addons/3rd/%(id)s/latest.xpi' % addon):
				filePath = '../addons/3rd/%(id)s/latest.xpi' % addon
			else:
				print('%sExtension %s not exist%s' % (Fore.RED, addon['id'], Fore.RESET))
				sys.exit(0)
			cmd = 'cp -p %s build/distribution/myextensions/%s.xpi' % (filePath, addon['id'])
			shell_command(cmd)
			section = 'Extension%d' % index
			cp_myext.add_section(section)
			cp_myext.set(section, 'id', addon['id'])
			cp_myext.set(section, 'file', '%s.xpi' % addon['id'])
			cp_myext.set(section, 'os', 'all')
			index += 1
			if extension_xml:
				extension_data[addon['id']]['file'] = addon['id']
				jinja2_context['default'].append(extension_data[addon['id']])
				self.picList.append(extension_data[addon['id']]['picture'])

		for addon in self.addonList['other']:
			if os.path.exists('../addons/official/%(id)s/%(branch)s/latest.xpi' % addon):
				filePath = '../addons/official/%(id)s/%(branch)s/latest.xpi' % addon
			elif os.path.exists('../addons/3rd/%(id)s/latest.xpi' % addon):
				filePath = '../addons/3rd/%(id)s/latest.xpi' % addon
			elif addon['id'] == 'null':
				print('%sid: null, ignore%s' % (Fore.CYAN, Fore.RESET))
			else:
				print('%sExtension %s not exist%s' % (Fore.RED, addon['id'], Fore.RESET))
				sys.exit(0)
			cmd = 'cp -p %s build/myextensions/%s.xpi' % (filePath, addon['id'])
			shell_command(cmd)
			section = 'Extension%d' % index
			cp_myext.add_section(section)
			cp_myext.set(section, 'id', addon['id'])
			cp_myext.set(section, 'file', '%s.xpi' % addon['id'])
			cp_myext.set(section, 'os', 'all')
			index += 1
			if extension_xml:
				extension_data[addon['id']]['file'] = addon['id']
				jinja2_context['other'].append(extension_data[addon['id']])
				self.picList.append(extension_data[addon['id']]['picture'])

		if extension_xml:
			jinja2_env = Environment(loader=FileSystemLoader('../common/res/%s' % self.locale))
			jinja2_template = jinja2_env.get_template('extensions.xml')
			extension_xml.write(jinja2_template.render(jinja2_context))
			extension_xml.close()

		fw = open('build/distribution/myextensions/config.ini','wb')
		cp_myext.write(fw, False)
		fw.close()

	def	MakePackage(self):
		print ('%sSelf-extracting installer%s' % (Fore.CYAN, Fore.RESET))
		shell_command('rm -rf build/tmp/core/distribution')
		shell_command('mv build/distribution/ build/tmp/core/')
		if self.template:
			shell_command('cp -p %s/* build/tmp/' % self.template)
		shell_command('7z.exe a -r -t7z -mx -m0=BCJ2 -m1=LZMA:d24 -m2=LZMA:d19 -m3=LZMA:d19 -mb0:1 -mb0s1:2 -mb0s2:3 build/tmp/installer.7z ./build/tmp/* >>null')
		shell_command('cp -p tools/7zS.sfx build/tmp')
		if os.path.exists('config/%s/app.tag' % self.channel):
			shell_command('cp -p config/%s/app.tag build/tmp/config.txt' % self.channel)
		else:
			shell_command('cp -p tools/config.txt build/tmp')
		shell_command('cp -p tools/make.bat build/tmp')
		cmd = os.path.join('build', 'tmp', 'make.bat')
		shell_command(cmd)

		if os.path.exists('../common/authenticode/SignBatch/SignBatch.bat'):
			cmd = os.path.join('..', 'common', 'authenticode', 'SignBatch', 'SignBatch.bat build/firefox-full.exe %s' % self.locale)
			shell_command(cmd)

	def MakeSetup(self):
		if self.rebuildFlag == '0':
			return
		print ('%sRe-compile setup.exe%s' % (Fore.CYAN, Fore.RESET))
		fw = open(self.scriptDir + '/mo-config.nsh','wb')
		fw.write('!define AppVersion	"%s"\r\n' % self.appVer)
		fw.write('!define AB_CD	"%s"\r\n' % self.locale)
		fw.write('!define MOResUrl "%s"\r\n' % self.resUrl)
		fw.write('!define MOExtUrl "%s"\r\n' % self.extUrl)
		fw.close()
		shell_command('makensisu-2.46.exe %s/installer.nsi >>null' % self.scriptDir)
		shell_command('mv %s/setup.exe build/' % self.scriptDir)
		if os.path.exists('../common/authenticode/SignBatch/SignBatch.bat'):
			cmd = os.path.join('..', 'common', 'authenticode', 'SignBatch', 'SignBatch.bat build/setup.exe %s' % self.locale)
			shell_command(cmd)
		shell_command('cp -p build/setup.exe build/tmp/')

	def MakeResPackage(self):
		if self.bundle != '1':
			return
		print ('%sPrepare res.7z%s' % (Fore.CYAN, Fore.RESET))
		for bmp in self.picList:
			shell_command('cp -p ../common/res/%s/%s build/res/' % (self.locale, bmp))
		shell_command('7z.exe a -r -t7z -mx -m0=BCJ2 -m1=LZMA:d24 -m2=LZMA:d19 -m3=LZMA:d19 -mb0:1 -mb0s1:2 -mb0s2:3 build/res.7z ./build/res/* >>null')

	def MakeMyextensions(self):
		if self.bundle != '1':
			return
		print ('%sPrepare myextensions.7z%s' % (Fore.CYAN, Fore.RESET))
		shell_command('7z.exe a -r -t7z -mx -m0=BCJ2 -m1=LZMA:d24 -m2=LZMA:d19 -m3=LZMA:d19 -mb0:1 -mb0s1:2 -mb0s2:3 build/myextensions.7z ./build/myextensions/* >>null')

	def UnpackPackage(self):
		self.DownloadSetup()
		print ('%sUnpack original installer%s' % (Fore.CYAN, Fore.RESET))
		cmd = '7z.exe x "../common/firefox/%(ver)s/%(locale)s/Firefox Setup %(ver)s.exe" -obuild/tmp/ >>null' % {'locale':self.locale, 'ver':self.version}
		shell_command(cmd)
		if os.path.exists('config/%s/bookmarks.html' % self.channel):
			mkdir('build/tmp/core/defaults/profile')
			cmd = 'cp -p config/%s/bookmarks.html build/tmp/core/defaults/profile/' % self.channel
			shell_command(cmd)

	def MoveToOutput(self):
		print ('%sMove to output folder%s' % (Fore.CYAN, Fore.RESET))
		ver_chl = {'ver':self.version, 'channel':self.channel}
		mkdir(os.path.join('out', self.version, self.channel))
		if self.bundle != '1':
			shell_command('cp build/*.exe out/%(ver)s/%(channel)s/' % ver_chl)
			mkdir(os.path.dirname('../output%s' % get_path(self.packageUrl)))
			shell_command('mv build/firefox-full.exe "../output%s"' % get_path(self.packageUrl))
		else:
			shell_command('cp build/*.{7z,exe} out/%(ver)s/%(channel)s/' % ver_chl)
			mkdir(os.path.dirname('../output%s' % get_path(self.packageUrl)))
			shell_command('mv build/firefox-full.exe "../output%s"' % get_path(self.packageUrl))
			mkdir(os.path.dirname('../output%s' % get_path(self.resUrl)))
			shell_command('mv build/res.7z "../output%s"' % get_path(self.resUrl))
			mkdir(os.path.dirname('../output%s' % get_path(self.extUrl)))
			shell_command('mv build/myextensions.7z "../output%s"' % get_path(self.extUrl))
		if self.channel in ['official', 'taiwan']:
			version_prefix = '.'.join(self.version.split('.')[:2])
			print ('%sAdditional res.7z for web installer%s' % (Fore.CYAN, Fore.RESET))
			if self.channel == 'official':
				mkdir('../output/releases/webins3.0/%s/res/official/en-US' % version_prefix)
				shell_command('7z.exe a -r -t7z -mx -m0=BCJ2 -m1=LZMA:d24 -m2=LZMA:d19 -m3=LZMA:d19 -mb0:1 -mb0s1:2 -mb0s2:3 ../output/releases/webins3.0/%s/res/official/en-US/res.7z ../common/res/en-US/* >> null' % version_prefix)
			mkdir('../output/releases/webins3.0/res')
			shell_command('cp ../output/releases/webins3.0/{%s/res/official/,res/} -r' % version_prefix)

	def CleanUp(self):
		print ('%sClean up%s' % (Fore.CYAN, Fore.RESET))
		shell_command('rm -rf build/*')
		shell_command('rm -rf null')

	def DownloadSetup(self):
		local_path = os.path.join('..', 'common', 'firefox', self.version, self.locale, ('Firefox Setup ' + self.version + '.exe'))
		if not os.path.exists(local_path):
			print ('%sDownload intl installers from stage.mozilla.org%s' % (Fore.CYAN, Fore.RESET))
			url = 'http://stage.mozilla.org/pub/mozilla.org/firefox/releases/%(ver)s/win32/%(locale)s/Firefox Setup %(ver)s.exe' % {'ver': self.version, 'locale':self.locale}
			mkdir(os.path.join('..', 'common', 'firefox', self.version, self.locale))
			def dl_progress(count, blockSize, totalSize):
				sys.stdout.write('\r' + local_path + '...%d%%' % int(count * blockSize * 100 / totalSize))
				sys.stdout.flush()

			import urllib
			class MozCnURLopener(urllib.FancyURLopener):
				version = 'Mozilla/5.0 (compatible; MozillaOnlineRepackBot/20120326; +bzhao@mozilla.com)'
			urllib._urlopener = MozCnURLopener()
			urllib.urlretrieve(url.replace(' ','%20'), local_path, dl_progress)

def main():
	for channel in channels:
		maker = FullPackageMaker(channel, options.version)
		maker.ParseXML()

		maker.CleanUp()

		maker.UnpackPackage()
		maker.MakeSetup()
		maker.MakeDistributionFolder()
		maker.MakePackage()

		maker.MakeMyextensions()
		maker.MakeResPackage()
		maker.MoveToOutput()

if __name__ == '__main__':
	from optparse import OptionParser
	parser = OptionParser(usage="usage: %prog [options]")
	parser.add_option("-v",
					"--version",
					action="store",
					dest="version",
					help="Set the version number for repacking")
	(options, args) = parser.parse_args()
	if not options.version:
		parser.error('Must provide a version')

	channels = args if len(args) else os.listdir('config')
	init()
	main()
