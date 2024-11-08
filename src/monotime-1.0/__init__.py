#
# Copyright 2011 Google Inc. All Rights Reserved.
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
#
"""time.monotonic() for older versions of python.

If time.monotonic() doesn't already exist, this module monkey patches it
in so that people looking for it will find it.  That function is available
starting in python 3.3; with this extension, it becomes available in
earlier versions too.

Somewhere in your program you need to 'import monotime' first.  After
that, any module looking for time.monotonic() will find it successfully.
"""

from _monotime import monotonic


import time as _time
if not hasattr(_time, 'monotonic'):
  _time.monotonic = monotonic
