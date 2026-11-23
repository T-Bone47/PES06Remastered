//MapAnchors
//Given a list of data addresses (typically strings), finds every function that
//references them and decompiles those functions. This turns semantic anchors
//in the string table into concrete code entry points.
//Usage: -postScript MapAnchors <addrs.txt> <out.txt>
//@category Analysis
//@author rea

import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;

public class MapAnchors extends GhidraScript {

	@Override
	public void run() throws Exception {
		String[] args = getScriptArgs();
		if (args.length < 2) {
			printerr("usage: MapAnchors <addrs.txt> <out.txt>");
			return;
		}
		List<String> lines = java.nio.file.Files.readAllLines(new File(args[0]).toPath());

		DecompInterface decomp = new DecompInterface();
		DecompileOptions opts = new DecompileOptions();
		decomp.setOptions(opts);
		decomp.openProgram(currentProgram);

		PrintWriter out = new PrintWriter(new OutputStreamWriter(
			new FileOutputStream(args[1]), StandardCharsets.UTF_8));

		for (String raw : lines) {
			String t = raw.trim();
			if (t.isEmpty() || t.startsWith("#")) {
				continue;
			}
			int sp = t.indexOf(' ');
			String addrHex = (sp < 0) ? t : t.substring(0, sp);
			String note = (sp < 0) ? "" : t.substring(sp + 1).trim();
			long va = Long.parseLong(addrHex.replace("0x", ""), 16);
			Address addr = currentProgram.getAddressFactory().getDefaultAddressSpace()
					.getAddress(va);

			out.println();
			out.println("################ " + addrHex + "  " + note + " ################");
			Data d = currentProgram.getListing().getDataAt(addr);
			if (d != null) {
				Object v = d.getValue();
				out.println("// value: " + (v == null ? "(null)" : v.toString()));
			}

			Set<Function> owners = new LinkedHashSet<Function>();
			ReferenceIterator rit = currentProgram.getReferenceManager().getReferencesTo(addr);
			while (rit.hasNext()) {
				Reference r = rit.next();
				Address from = r.getFromAddress();
				Function owner = currentProgram.getFunctionManager().getFunctionContaining(from);
				out.println("  ref from " + from + " " + r.getReferenceType()
						+ (owner == null ? "" : "  in " + owner.getName()));
				if (owner != null) {
					owners.add(owner);
				}
			}
			out.println("  -> " + owners.size() + " referencing function(s)");
			for (Function f : owners) {
				out.println();
				out.println("  // ===== " + f.getName() + " @ " + f.getEntryPoint()
						+ "  size=" + f.getBody().getNumAddresses() + " =====");
				try {
					DecompileResults res = decomp.decompileFunction(f, 120, monitor);
					if (res != null && res.decompileCompleted()) {
						out.println(res.getDecompiledFunction().getC());
					}
					else {
						out.println("  (decompile failed)");
					}
				}
				catch (Exception e) {
					out.println("  (exception: " + e.getMessage() + ")");
				}
			}
			out.flush();
			println(addrHex + " -> " + owners.size() + " functions");
		}
		out.close();
		decomp.dispose();
		println("wrote " + args[1]);
	}
}
