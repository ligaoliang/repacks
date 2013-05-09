#!/usr/bin/python
from __future__ import print_function
from subprocess import Popen
from subprocess import call
import os, sys, json

from colorama import init, Fore, Back, Style

def mkdir(dir, mode=0755):
	if not os.path.exists(dir):
		return os.makedirs(dir, mode)
	return True

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

class WebInstallerMaker:
	""""WebInstallerMaker is a tool for making webinstallers"""
	def  __init__(self, locale, channel, path, version):
		print ('%sWeb Installer (locale: %s, channel: %s)%s' % (Fore.CYAN, locale, channel, Fore.RESET))
		self.locale = locale
		self.channel = channel
		self.path = path
		self.version = version
		if self.version:
			self.path = self.path.replace('webins3.0', 'webins3.0/%s' % self.version)

	def MakeSetups(self):
		print ('%sRe-compile setup.exe%s' % (Fore.CYAN, Fore.RESET))
		subd = 'webins3.0'
		if self.channel.startswith('esr'):
			subd = 'webins3.0/esr'
		if self.version:
			subd = 'webins3.0/%s' % self.version
		fw = open('scripts/%s/mo-config.nsh' % self.locale, 'wb')
		fw.write('!define MOChannel	"$\\"%s$\\""\r\n' % self.channel)
		fw.write('!define MOBaseInstallerUrl	"http://download.myfirefox.com.tw/releases/%s/firefox/official/%s/Firefox-latest.exe"\r\n' % (subd, self.locale))
		fw.write('!define MOResUrl	"http://download.myfirefox.com.tw/releases/%s/res/official/%s/res.7z"\r\n' % (subd, self.locale))
		fw.close()
		shell_command('makensisu-2.46.exe scripts/%s/installer.nsi >>null' % self.locale)
		shell_command('mv scripts/%s/setup.exe build/' % self.locale)
		if os.path.exists('../common/authenticode/SignBatch/SignBatch.bat'):
			cmd = os.path.join('..', 'common', 'authenticode', 'SignBatch', 'SignBatch.bat build/setup.exe %s' % self.locale)
			shell_command(cmd)
		mkdir('build/tmp')
		shell_command('cp -p build/setup.exe build/tmp/')

	def	MakePackages(self):
		print ('%sSelf-extracting installer%s' % (Fore.CYAN, Fore.RESET))
		shell_command('cp config/template/%s/* build/tmp/' % self.locale)
		shell_command('7z.exe a -r -t7z -mx -m0=BCJ2 -m1=LZMA:d24 -m2=LZMA:d19 -m3=LZMA:d19 -mb0:1 -mb0s1:2 -mb0s2:3 build/tmp/installer.7z ./build/tmp/* >>null')
		shell_command('cp -p tools/7zS.sfx build/tmp')
		shell_command('cp -p tools/config.txt build/tmp')
		shell_command('cp -p tools/make.bat build/tmp')
		cmd = os.path.join('build', 'tmp', 'make.bat')
		shell_command(cmd)

		if os.path.exists('../common/authenticode/SignBatch/SignBatch.bat'):
			cmd = os.path.join('..', 'common', 'authenticode', 'SignBatch', 'SignBatch.bat build/firefox.exe %s' % self.locale)
			shell_command(cmd)

	def MoveToOutput(self):
		print ('%sMoving to output folder%s' % (Fore.CYAN, Fore.RESET))
		mkdir(os.path.dirname('../output/%s' % self.path))
		shell_command('mv build/firefox.exe "../output/%s"' % self.path)

	def CleanUp(self):
		print ('%sCleaning%s' % (Fore.CYAN, Fore.RESET))
		shell_command('rm -rf build/*')
		shell_command('rm -rf null')

def main(locales, channels, version):
	locale_channel = json.load(open('config/channels.json'))
	if not locales:
		locales = locale_channel.keys()
	else:
		locales = [l for l in locales if l in locale_channel]
	for locale in locales:
		if not channels:
			channels = locale_channel[locale].keys()
		else:
			channels = [c for c in channels if c in locale_channel[locale]]
		for channel in channels:
			path = locale_channel[locale][channel]
			maker = WebInstallerMaker(locale, channel, path, version)
			maker.CleanUp()
			maker.MakeSetups()
			maker.MakePackages()
			maker.MoveToOutput()


if __name__ == '__main__':
	from optparse import OptionParser
	parser = OptionParser(usage="usage: %prog [options]")
	parser.add_option("-l",
					"--locale",
					action="store",
					dest="locales",
					help="Set the locales for repacking")
	parser.add_option("-v",
					"--version",
					action="store",
					dest="version",
					help="Versioned web installer")
	(options, args) = parser.parse_args()

	locales = options.locales.split(',') if options.locales else []
	channels = args if len(args) else []

	init()
	main(locales, channels, options.version)



