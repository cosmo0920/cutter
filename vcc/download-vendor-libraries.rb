#!/usr/bin/env ruby
#
# Copyright(C) 2010 Ruby-GNOME2 Project.
# Copyright(C) 2011 Kouhei Sutou <kou@clear-code.com>
#
# License: LGPL 2.1 or later.

require 'open-uri'
require 'rubygems'
require 'mechanize'

class Win32BinaryDownloader
  class << self
    def download(options)
      downloader = new(options)
      packages = options[:packages] || [options[:package]]
      packages.compact.each do |package|
        downloader.download_package(package)
      end
      dependencies = options[:dependencies] || [options[:dependency]]
      dependencies.compact.each do |dependency|
        downloader.download_dependency(dependency)
      end
    end
  end

  URL_BASE = "http://ftp.gnome.org/pub/gnome/binaries/win32"
  def initialize(options={})
    output_dir = options[:output_dir] || File.join("vendor", "local")
    @output_dir = File.expand_path(output_dir)
  end

  def download_package(package)
    version_page_url = "#{URL_BASE}/#{package}"
    version_page = agent.get(version_page_url)
    latest_version_link = version_page.links.sort_by do |link|
      if /\A(\d+\.\d+)\/\z/ =~ link.href
        $1.split(/\./).collect {|component| component.to_i}
      else
        [-1]
      end
    end.last

    escaped_package = Regexp.escape(package)
    latest_version_page = latest_version_link.click
    latest_version = latest_version_page.links.collect do |link|
      case link.href
      when /#{escaped_package}_([\d\.\-]+)_win32\.zip\z/,
           /#{escaped_package}-([\d\.\-]+)-win32\.zip\z/, # old
           /#{escaped_package}-([\d\.\-]+)\.zip\z/ # old
        version = $1
        normalized_version = version.split(/[\.\-]/).collect do |component|
          component.to_i
        end
        [normalized_version, version]
      else
        [[-1], nil]
      end
    end.sort_by do |normalized_version, version|
      normalized_version
    end.last[1]

    if latest_version.nil?
      raise "can't find package: <#{package}>:<#{version_page_url}>"
    end
    escaped_latest_version = Regexp.escape(latest_version)
    latest_version_page.links.each do |link|
      case link.href
      when /#{escaped_package}(?:-dev)?_#{escaped_latest_version}_win32\.zip\z/,
           /#{escaped_package}(?:-dev)?-#{escaped_latest_version}-win32\.zip\z/, # old
           /#{escaped_package}(?:-dev)?-#{escaped_latest_version}\.zip\z/ # old
        click_zip_link(link)
      end
    end
  end

  def download_dependency(dependency)
    dependency_version = "any"
    dependency_version_re = /[\d\.\-]+/
    if dependency.is_a?(Array)
      dependency, dependency_version = dependency
      dependency_version_re = /#{Regexp.escape(dependency_version)}/
    end
    escaped_dependency = Regexp.escape(dependency)
    dependencies_url = "#{URL_BASE}/dependencies"
    dependencies_page = agent.get(dependencies_url)
    latest_version = dependencies_page.links.collect do |link|
      case link.href
      when /\A#{escaped_dependency}_(#{dependency_version_re})_win32\.zip\z/
        version = $1
        [version.split(/[\.\-]/).collect {|component| component.to_i}, version]
      else
        [[-1], nil]
      end
    end.sort_by do |normalized_version, version|
      normalized_version
    end.last[1]

    if latest_version.nil?
      message = "can't find dependency package: " +
        "<#{dependency}>(#{dependency_version}):<#{dependencies_url}>"
      raise message
    end
    escaped_latest_version = Regexp.escape(latest_version)
    dependencies_page.links.each do |link|
      case link.href
      when /\A#{escaped_dependency}(?:-dev)?_#{escaped_latest_version}_win32.zip/
        click_zip_link(link)
      end
    end
  end

  private
  def agent
    @agent ||= Mechanize.new
  end

  def click_zip_link(link)
    zip = link.click
    FileUtils.mkdir_p(@output_dir)
    Dir.chdir(@output_dir) do
      open(zip.filename, "wb") do |file|
        file.print(zip.body)
      end
      system("unzip", "-o", zip.filename)
      Dir.glob("lib/pkgconfig/*.pc") do |pc_path|
        pc = File.read(pc_path)
        pc = pc.gsub(/\Aprefix=.+$/) {"prefix=#{@output_dir}"}
        File.open(pc_path, "w") do |pc_file|
          pc_file.print(pc)
        end
      end
    end
  end
end

options = {
  :output_dir => File.join(File.dirname(__FILE__), "vendor"),
  :packages => ["glib", "atk", "pango", "gdk-pixbuf", "gtk+", "libsoup"],
  :dependencies => ["gettext-runtime", "zlib", "libpng", "cairo"],
}
Win32BinaryDownloader.download(options)
