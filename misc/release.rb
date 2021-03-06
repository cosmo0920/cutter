#!/usr/bin/env ruby

require 'English'
require 'rubygems'
require 'mechanize'
require 'logger'

if ARGV.size < 7
  puts "Usage: #{$0} " +
         "SF_USER_NAME PROJECT_ID PROJECT_NAME PACKAGE_NAME RELEASE_NAME " +
         "README NEWS FILES..."
  puts " e.g.: #{$0} ktou cutter Cutter cutter 0.3.0 README NEWS " +
         "cutter-0.3.0.tar.gz cutter-0.3.0.tar.bz2"
  exit(1)
end

sf_user_name, project_id, project_name, package_name, release_name, \
  readme, news, *files = ARGV

def read_password(prompt, input=$stdin, output=$stdout)
  output.print(prompt)
  system("/bin/stty -echo") if input.tty?
  input.gets.chomp
ensure
  system("/bin/stty echo") if input.tty?
  output.puts
end

def login(agent, user_name, password=nil)
  page = agent.get("https://sourceforge.net/auth/")

  login_form = page.forms.find {|form| /login/ =~ form.action}
  login_form.username = user_name
  login_form.password = password || yield

  begin
    page = agent.submit(login_form, login_form.buttons.first)
  rescue Mechanize::ResponseCodeError
  end
  raise "login failed" unless /Log Out|Log&nbsp;out/ =~ page.body
  page = agent.get("https://sourceforge.net/users/#{user_name}")
  page
end

def go_project_page(agent, my_page, project_name)
  project_page_link = my_page.links.find do |link|
    /\A#{Regexp.escape(project_name)}\z/i =~ link.text
  end
  agent.click(project_page_link)
end

def upload_files(sf_user_name, project_id, package_name, release_name,
                 news, files)
  dist_top_dir = "dist"
  FileUtils.rm_rf(dist_top_dir)
  dist_dir = File.join(dist_top_dir, package_name, release_name)
  FileUtils.mkdir_p(dist_dir)
  FileUtils.cp(files, dist_dir)
  File.open(File.join(dist_dir, "README.txt"), "w") do |readme|
    readme.print(latest_release_changes(news))
  end
  Dir.chdir(dist_top_dir) do
    host = "#{sf_user_name},#{project_id}@frs.sourceforge.net"
    path = ["", "home", "frs", "project",
            project_id[0, 1], project_id[0, 2], project_id].join("/")
    system("rsync", "-avz", "./", "#{host}:#{path}/")
  end
end

def go_development_page(agent, project_page, project_id)
  development_page_link = project_page.links.find do |link|
    /\bprojects\b/ =~ link.href and /\bDevelop\b/ =~ link.text
  end
  if development_page_link.nil?
    agent.get("https://sourceforge.net/projects/#{project_id}/develop")
  else
    agent.click(development_page_link)
  end
end

def extract_sections(file)
  normalized_text = File.read(file).gsub(/[=#]{2,}\n.*\n[=#]{2,}\n/, '')
  normalized_text.split(/.*\n^(?:[=#]{2}\s+|[=#]+$).*\n\n\n*/)
end

def guess_target_index(file, default_index)
  index = default_index
  if /:(\d+)\Z/ =~ file
    file = $PREMATCH
    index = $1.to_i
  end
  [file, index]
end

def project_summary(readme)
  readme, index = guess_target_index(readme, 4)
  extract_sections(readme)[index].split(/\n\n/)[0].chomp
end

def latest_release_changes(news)
  news, index = guess_target_index(news, 1)
  extract_sections(news)[index].chomp
end

def remove_rd_link_markup(text)
  text.gsub(/\(\(<(.*?)(?:\|.*?)?>\)\)/m, '\1')
end

def go_news_page(agent, development_page)
  agent.click(development_page.links.find {|link| /\ANews\z/ =~ link.text})
end

def go_submit_news_page(agent, news_page)
  submit_news_page_link = news_page.links.find do |link|
    /\bnews\b/ =~ link.href and /\ANew Post\z/ =~ link.text
  end
  agent.click(submit_news_page_link)
end

def submit_news(agent, submit_news_page, project_name, package_name,
                release_name, readme, news)
  submit_news_page.form_with(:action => "save") do |form|
    title = "#{project_name}: #{package_name} #{release_name} Released"
    form.title = title
    details = [project_summary(readme),
               latest_release_changes(news)].join("\n\n")
    form.text = remove_rd_link_markup(details).gsub(/\n/, "\r\n")
    form.submit
  end
end

def main(sf_user_name, project_id, project_name,
         package_name, release_name, readme, news, files)
  agent = Mechanize.new do |_agent|
    # _agent.log = Logger.new(STDOUT)
  end
  password = read_password("SF.net password for [#{sf_user_name}]: ")
  my_page = login(agent, sf_user_name, password)

  project_page = go_project_page(agent, my_page, project_name)
  development_page = go_development_page(agent, project_page, project_id)
  upload_files(sf_user_name, project_id, package_name,
               release_name, news, files)

  news_page = go_news_page(agent, development_page)
  submit_news_page = go_submit_news_page(agent, news_page)
  submit_news(agent, submit_news_page, project_name, package_name,
              release_name, readme, news)
end

main(sf_user_name, project_id, project_name,
     package_name, release_name, readme, news, files)
