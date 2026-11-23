//DumpTargetFunctions
//Dumps decompilation + metadata for every PESMod target address in one headless run.
//@category Analysis
//@author rea

import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.mem.Memory;

public class DumpTargetFunctions extends GhidraScript {

	private static final int DECOMPILE_TIMEOUT_SECONDS = 120;

	private static String j(String s) {
		if (s == null) {
			return "";
		}
		StringBuilder b = new StringBuilder();
		for (int i = 0; i < s.length(); i++) {
			char c = s.charAt(i);
			switch (c) {
				case '"':
					b.append("\\\"");
					break;
				case '\\':
					b.append("\\\\");
					break;
				case '\n':
					b.append("\\n");
					break;
				case '\r':
					b.append("\\r");
					break;
				case '\t':
					b.append("\\t");
					break;
				default:
					if (c < 0x20) {
						b.append(String.format("\\u%04x", (int) c));
					}
					else {
						b.append(c);
					}
			}
		}
		return b.toString();
	}

	/** All function entry points, ascending, so we can find the nearest preceding one. */
	private List<Function> allFunctions = new ArrayList<Function>();

	private Function nearestAtOrBefore(Address addr) {
		Function best = null;
		long bestAddr = -1;
		long target = addr.getOffset();
		for (Function f : allFunctions) {
			long off = f.getEntryPoint().getOffset();
			if (off <= target && off > bestAddr) {
				bestAddr = off;
				best = f;
			}
		}
		return best;
	}

	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		if (args.length < 2) {
			printerr("usage: DumpTargetFunctions <targets.txt> <out.json> [functions.tsv] [strings.tsv]");
			return;
		}
		String targetFile = args[0];
		String outFile = args[1];
		String funcFile = args.length > 2 ? args[2] : null;
		String strFile = args.length > 3 ? args[3] : null;

		List<String[]> targets = new ArrayList<String[]>();
		for (String line : java.nio.file.Files.readAllLines(new File(targetFile).toPath())) {
			String t = line.trim();
			if (t.isEmpty() || t.startsWith("#")) {
				continue;
			}
			int tab = t.indexOf('\t');
			if (tab < 0) {
				tab = t.indexOf(' ');
			}
			if (tab < 0) {
				continue;
			}
			targets.add(new String[] { t.substring(0, tab).trim(), t.substring(tab + 1).trim() });
		}
		println("DumpTargetFunctions: " + targets.size() + " targets");

		FunctionIterator fit = currentProgram.getFunctionManager().getFunctions(true);
		while (fit.hasNext()) {
			allFunctions.add(fit.next());
		}
		println("functions in program: " + allFunctions.size());

		DecompInterface decomp = new DecompInterface();
		try {
			DecompileOptions opts = new DecompileOptions();
			decomp.setOptions(opts);
			if (!decomp.openProgram(currentProgram)) {
				printerr("decompiler failed to open program");
				return;
			}

			PrintWriter out = new PrintWriter(new OutputStreamWriter(
				new FileOutputStream(outFile), StandardCharsets.UTF_8));
			out.println("{");
			out.println("  \"program\": \"" + j(currentProgram.getName()) + "\",");
			out.println("  \"imageBase\": \"" + j(currentProgram.getImageBase().toString()) + "\",");
			out.println("  \"functionCount\": " + allFunctions.size() + ",");
			out.println("  \"targets\": [");

			for (int i = 0; i < targets.size(); i++) {
				String vaHex = targets.get(i)[0];
				String label = targets.get(i)[1];
				long va = Long.parseLong(vaHex.replace("0x", ""), 16);
				Address addr = currentProgram.getAddressFactory().getDefaultAddressSpace()
						.getAddress(va);

				StringBuilder rec = new StringBuilder();
				rec.append("    {");
				rec.append("\"va\": \"").append(vaHex).append("\", ");
				rec.append("\"label\": \"").append(j(label)).append("\"");

				Function fn = currentProgram.getFunctionManager().getFunctionContaining(addr);
				Function exact = currentProgram.getFunctionManager().getFunctionAt(addr);

				if (fn == null) {
					Function near = nearestAtOrBefore(addr);
					rec.append(", \"status\": \"no_function\"");
					rec.append(", \"nearest\": \"").append(
						near == null ? "none"
								: j(near.getEntryPoint().toString() + " " + near.getName()))
						.append("\"");
					rec.append(", \"distance\": ").append(
						near == null ? -1 : (va - near.getEntryPoint().getOffset()));
					rec.append(", \"bytes\": \"").append(j(hexAt(addr, 16))).append("\"");
				}
				else {
					rec.append(", \"status\": \"ok\"");
					rec.append(", \"function\": \"").append(j(fn.getName())).append("\"");
					rec.append(", \"entry\": \"").append(j(fn.getEntryPoint().toString())).append("\"");
					rec.append(", \"signature\": \"").append(j(fn.getSignature().toString())).append("\"");
					rec.append(", \"size\": ").append(fn.getBody().getNumAddresses());
					rec.append(", \"exact_entry\": ").append(exact != null);
					rec.append(", \"callingConvention\": \"").append(j(fn.getCallingConventionName()))
							.append("\"");
					rec.append(", \"nparams\": ").append(fn.getParameterCount());
					rec.append(", \"thunk\": ").append(fn.isThunk());
					rec.append(", \"external\": ").append(fn.isExternal());
					try {
						rec.append(", \"callers\": ").append(fn.getCallingFunctions(monitor).size());
						rec.append(", \"callees\": ").append(fn.getCalledFunctions(monitor).size());
					}
					catch (Exception e) {
						rec.append(", \"callers\": 0, \"callees\": 0");
					}
					if (exact == null) {
						rec.append(", \"offset_into_function\": ")
								.append(addr.subtract(fn.getEntryPoint()));
					}
					try {
						DecompileResults res = decomp.decompileFunction(fn, DECOMPILE_TIMEOUT_SECONDS,
							monitor);
						if (res != null && res.decompileCompleted()) {
							rec.append(", \"pseudocode\": \"")
									.append(j(res.getDecompiledFunction().getC())).append("\"");
						}
						else {
							rec.append(", \"pseudocode\": null");
							if (res != null && res.getErrorMessage() != null) {
								rec.append(", \"decompileError\": \"")
										.append(j(res.getErrorMessage())).append("\"");
							}
						}
					}
					catch (Exception e) {
						rec.append(", \"pseudocode\": null, \"decompileError\": \"")
								.append(j(String.valueOf(e.getMessage()))).append("\"");
					}
				}
				rec.append("}");
				if (i < targets.size() - 1) {
					rec.append(",");
				}
				out.println(rec.toString());
				out.flush();
				println("[" + (i + 1) + "/" + targets.size() + "] " + vaHex + " " + label + " -> "
						+ (fn != null ? fn.getName() : "NO FUNCTION"));
			}
			out.println("  ]");
			out.println("}");
			out.close();
			println("wrote " + outFile);
		}
		finally {
			decomp.dispose();
		}

		if (funcFile != null) {
			PrintWriter fw = new PrintWriter(new OutputStreamWriter(
				new FileOutputStream(funcFile), StandardCharsets.UTF_8));
			fw.println("entry\tname\tsize\tsignature");
			List<Function> sorted = new ArrayList<Function>(allFunctions);
			Collections.sort(sorted, new Comparator<Function>() {
				@Override
				public int compare(Function a, Function b) {
					return Long.compare(a.getEntryPoint().getOffset(), b.getEntryPoint().getOffset());
				}
			});
			for (Function f : sorted) {
				fw.println(f.getEntryPoint() + "\t" + f.getName() + "\t"
						+ f.getBody().getNumAddresses() + "\t"
						+ f.getSignature().toString().replace("\t", " ").replace("\n", " "));
			}
			fw.close();
			println("wrote " + funcFile);
		}

		if (strFile != null) {
			PrintWriter sw = new PrintWriter(new OutputStreamWriter(
				new FileOutputStream(strFile), StandardCharsets.UTF_8));
			sw.println("address\tvalue");
			Listing listing = currentProgram.getListing();
			ghidra.program.model.listing.DataIterator dit = listing.getDefinedData(true);
			while (dit.hasNext()) {
				Data d = dit.next();
				Object v = d.getValue();
				if (v == null) {
					continue;
				}
				String s = String.valueOf(v);
				if (s.isEmpty()) {
					continue;
				}
				sw.println(d.getAddress() + "\t"
						+ s.replace("\n", "\\n").replace("\r", "\\r").replace("\t", "\\t"));
			}
			sw.close();
			println("wrote " + strFile);
		}
	}

	private String hexAt(Address addr, int n) {
		try {
			Memory mem = currentProgram.getMemory();
			byte[] b = new byte[n];
			int got = mem.getBytes(addr, b, 0, n);
			StringBuilder sb = new StringBuilder();
			for (int i = 0; i < got; i++) {
				if (i > 0) {
					sb.append(' ');
				}
				sb.append(String.format("%02x", b[i]));
			}
			return sb.toString();
		}
		catch (Exception e) {
			return "unreadable";
		}
	}
}
