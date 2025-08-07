use std::fs::{File, write};
use std::io::Write;
use std::process::{Command, Stdio};
use std::thread::sleep;
use std::time::Duration;

pub fn run_program(program_path: &str, trace_path: &str) {
    println!("[*] Starting QEMU...");

    let mut qemu_child = Command::new("qemu-riscv64")
        .arg("-g")
        .arg("1234")
        .arg("-singlestep")
        .arg(program_path)
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .spawn()
        .expect("Failed to start QEMU");

    sleep(Duration::from_secs(1));

    println!("[*] Writing GDB script...");
    let mut gdb_script = File::create("./trace.gdb").unwrap();
    writeln!(gdb_script, "target remote localhost:1234").unwrap();
    writeln!(gdb_script, "set pagination off").unwrap();
    writeln!(gdb_script, "set confirm off").unwrap();
    writeln!(gdb_script, "set disassemble-next-line on").unwrap();
    writeln!(gdb_script, "set $pc = *main").unwrap();
    writeln!(gdb_script, "set $done = 0").unwrap();
    writeln!(gdb_script, "set $i = 0").unwrap();
    writeln!(gdb_script, "define do_step").unwrap();
    writeln!(gdb_script, "  si").unwrap();
    writeln!(gdb_script, "  printf \"\\nPC: 0x%x\\n\", $pc").unwrap();
    writeln!(gdb_script, "  x/i $pc").unwrap();
    writeln!(gdb_script, "  info registers").unwrap();
    writeln!(gdb_script, "  set $i = $i + 1").unwrap();
    writeln!(gdb_script, "  if $i >= 1000").unwrap();
    writeln!(gdb_script, "    set $done = 1").unwrap();
    writeln!(gdb_script, "  end").unwrap();
    writeln!(gdb_script, "end").unwrap();
    writeln!(gdb_script, "while !$done").unwrap();
    writeln!(gdb_script, "  do_step").unwrap();
    writeln!(gdb_script, "end").unwrap();
    writeln!(gdb_script, "quit").unwrap();

    println!("[*] Running GDB...");
    let output = Command::new("riscv64-unknown-elf-gdb")
        .arg("-q")
        .arg(program_path)
        .arg("-x")
        .arg("./trace.gdb")
        .output()
        .expect("Failed to run GDB");

    if !output.status.success() {
        eprintln!("GDB exited with error:\n{}", String::from_utf8_lossy(&output.stderr));
    }

    let trace = String::from_utf8_lossy(&output.stdout);
    write("./traces/trace_raw.log", trace.as_bytes()).unwrap();

    println!("[*] Parsing trace...");
    let parsed = parse_trace(&trace);
    write(trace_path, parsed).unwrap();

    println!("[*] Cleaning up...");
    let _ = qemu_child.kill();
    let _ = qemu_child.wait();

    println!("[*] Done. Output written to {}", trace_path);
}


fn parse_trace(raw: &str) -> String {
    use std::collections::HashMap;

    let mut result = String::new();
    let mut lines = raw.lines().peekable();
    let mut current_instr = None;

    while let Some(line) = lines.next() {
        let trimmed = line.trim();

        // Detect instruction line (e.g., starts with address + tab)
        if trimmed.contains(':') && trimmed.contains('\t') {
            current_instr = Some(trimmed.to_string());
        } else if trimmed.starts_with("x0") || trimmed.starts_with("ra") || trimmed.starts_with("sp") {
            let mut xregs: HashMap<String, String> = HashMap::new();

            // Collect ~32 lines of register values
            xregs.extend(
                std::iter::once(trimmed)
                    .chain(lines.by_ref().take(33))
                    .filter_map(|reg_line| {
                        let parts = reg_line.trim().split_whitespace().collect::<Vec<_>>();
                        if parts.len() >= 2 {
                            let reg = parts[0];
                            let val = parts[1].trim_start_matches("0x");
                            if let Some(xname) = map_to_x_register(reg) {
                                return Some((xname, format!("0x{:08x}", u64::from_str_radix(val, 16).unwrap_or(0))));
                            }
                        }
                        None
                    }),
            );

            if let Some(instr) = &current_instr {
                // Clean and normalize instruction line:
                let raw_instr = instr.trim_start_matches("=>").trim();
                let parts: Vec<&str> = raw_instr.splitn(2, ':').collect();
                let pc_with_label = parts.get(0).map(|s| s.trim()).unwrap_or("");
                let disasm = parts.get(1).map(|s| s.trim()).unwrap_or("");

                // Strip anything after the PC (like <main()+2>)
                let pc = pc_with_label.split_whitespace().next().unwrap_or(pc_with_label);


                // Replace register aliases with xN form
                let clean_disasm = replace_aliases_with_x(disasm);

                // First dump registers
                for i in 0..32 {
                    let reg = format!("x{}", i);
                    let val = if reg == "x0" {
                        "0x00000000".to_string()
                    } else {
                        xregs.get(&reg).cloned().unwrap_or_else(|| "--------".to_string())
                    };
                    result.push_str(&format!("{}={} ", reg, val));
                }
                result.push('\n');

                // Add PC line (normalize to 32-bit or full 64-bit)
                result.push_str(&format!("{}: {}\n", pc, clean_disasm));
            }

            current_instr = None;
        }
    }

    let mut lines: Vec<&str> = result.lines().collect();

    // Remove last two lines if they exist
    if lines.len() >= 1 {
        lines.truncate(lines.len() - 1);
    }

    // Reconstruct result and add final empty line
    result = lines.join("\n") + "\n";

    result
}



fn replace_aliases_with_x(instr: &str) -> String {
    let reg_map: [(&str, &str); 33] = [
        ("zero", "x0"), ("ra", "x1"), ("sp", "x2"), ("gp", "x3"), ("tp", "x4"),
        ("t0", "x5"), ("t1", "x6"), ("t2", "x7"), ("s0", "x8"), ("fp", "x8"), ("s1", "x9"),
        ("a0", "x10"), ("a1", "x11"), ("a2", "x12"), ("a3", "x13"), ("a4", "x14"),
        ("a5", "x15"), ("a6", "x16"), ("a7", "x17"), ("s2", "x18"), ("s3", "x19"),
        ("s4", "x20"), ("s5", "x21"), ("s6", "x22"), ("s7", "x23"), ("s8", "x24"),
        ("s9", "x25"), ("s10", "x26"), ("s11", "x27"), ("t3", "x28"), ("t4", "x29"),
        ("t5", "x30"), ("t6", "x31"),
    ];

    let mut replaced = instr.to_string();
    for (alias, xname) in reg_map {
        replaced = replaced.replace(&format!("{}(", alias), &format!("{}(", xname));
        replaced = replaced.replace(&format!(", {}", alias), &format!(", {}", xname));
        replaced = replaced.replace(&format!(" {}", alias), &format!(" {}", xname));
        replaced = replaced.replace(&format!("\t{}", alias), &format!("\t{}", xname));
        replaced = replaced.replace(&format!("({})", alias), &format!("({})", xname));
        replaced = replaced.replace(&format!("{},", alias), &format!("{},", xname));
    }
    replaced
}

fn map_to_x_register(name: &str) -> Option<String> {
    let reg_map = [
        ("zero", 0), ("ra", 1), ("sp", 2), ("gp", 3), ("tp", 4),
        ("t0", 5), ("t1", 6), ("t2", 7), ("s0", 8), ("fp", 8), ("s1", 9),
        ("a0", 10), ("a1", 11), ("a2", 12), ("a3", 13), ("a4", 14),
        ("a5", 15), ("a6", 16), ("a7", 17), ("s2", 18), ("s3", 19),
        ("s4", 20), ("s5", 21), ("s6", 22), ("s7", 23), ("s8", 24),
        ("s9", 25), ("s10", 26), ("s11", 27), ("t3", 28), ("t4", 29),
        ("t5", 30), ("t6", 31),
    ];

    for (alias, xidx) in reg_map {
        if alias == name {
            return Some(format!("x{}", xidx));
        }
    }

    if name.starts_with('x') && name[1..].parse::<usize>().is_ok() {
        return Some(name.to_string());
    }

    None
}
