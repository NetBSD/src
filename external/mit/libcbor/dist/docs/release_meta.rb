require 'json'
require 'open-uri'

class Release
	def self.tag
		@@release ||= JSON.load(open('https://api.github.com/repos/pjk/libcbor/releases')).first
		@@release['tag_name']
	end

	def self.version
		tag.sub('v', '')
	end
end
