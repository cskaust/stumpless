#!/usr/bin/env ruby

# frozen_string_literal: true

# Copyright 2022 Joel E. Anderson
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# validates the localization (l10n) definitions in the given files, including:
#   - the l10n string definitions all occur in alphabetical order
#   - all headers have the same l10n strings defined
#   - lines do not go longer than 80 characters
#   - multiline string literals (lines ending with " \) do not have a space
#     at the end of the first string, and do have a space as the first character
#     of the following lines
#   - l10n strings marked as needing translation (via a preceding comment of
#     "// todo translate") have the english translation as their value. This
#     is only done if the english locale file is one of those checked.

return_code = 0
header_defines = {}
defaults_header = 'en-us.h'
defaults = nil

ARGV.each do |source_glob|
  Dir.glob(source_glob) do |source_filename|
    file_defines = {}
    current_l10n = nil
    str = String.new

    File.open(source_filename).each do |line|
      m = line.match(/^"(.*)"/)
      if current_l10n && m
        str << m[1]

        unless line.end_with?('\\')
          file_defines[current_l10n] = str
          current_l10n = nil
          str = String.new
        end
      end

      m = line.match(/#\s*define\s*L10N_(\w*)\s/)
      current_l10n = m[1] if m
    end

    header_defines[source_filename] = file_defines
    defaults = file_defines if source_filename.end_with?(defaults_header)
  end
end

exit return_code
