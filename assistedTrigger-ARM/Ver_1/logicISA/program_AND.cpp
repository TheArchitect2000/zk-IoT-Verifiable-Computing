// Copyright 2025 Fidesinnova.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

int main() {
    asm volatile (
        "mov x17, #3\n"
        "mov x18, #8\n"
        "mov x19, #15\n"
        "mov x20, #2\n"
        "mov x21, #6\n"
        "and x17, x18, x19\n"
        "and x18, x19, x20\n"
        "and x19, x17, x21\n"
        "and x20, x18, x21\n"
    );
    return 0;
}