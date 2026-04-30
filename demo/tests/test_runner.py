import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from demo.runner import build_demo_kripke, kripke_to_dot, parse_case_output


class RunnerTests(unittest.TestCase):
    def test_build_demo_kripke_uses_requested_size(self):
        kripke = build_demo_kripke(state_count=5, candidate_count=3, voter_count=2)

        self.assertEqual(len(kripke["states"]), 5)
        self.assertEqual(kripke["initialStates"], ["s0"])
        self.assertEqual(kripke["transitions"][0], {"from": "s0", "to": "s1"})
        self.assertIn("candidate_3", kripke["propLabels"]["s2"])

    def test_kripke_to_dot_marks_initial_state_and_labels(self):
        kripke = build_demo_kripke(state_count=3, candidate_count=2, voter_count=1)

        dot = kripke_to_dot(kripke)

        self.assertIn("digraph ProbeMachine", dot)
        self.assertIn('"s0" -> "s1";', dot)
        self.assertIn("shape=doublecircle", dot)
        self.assertIn("candidate_2", dot)

    def test_parse_case_output_returns_frontend_summary(self):
        output = """
************* Data Library X ****************
s0-omega-s2	s1-s0-s3
************ Probe Library Y ****************
<s0,s1>  <s1,s3>
**************** Step1 ********************
s0s1-omega-s3	s1s3-s0-s5
**************** Step2 ********************
s0s1s3-omega-s5
********************************
voters:    1
candidates:2
agents:    1
**************** Result ****************
True
Path is: s0s1s2
Time taken: 0.123 seconds
"""

        summary = parse_case_output(output)

        self.assertEqual(summary["result"], "True")
        self.assertEqual(summary["path"], "s0s1s2")
        self.assertEqual(summary["seconds"], 0.123)
        self.assertEqual(summary["parameters"]["voters"], 1)
        self.assertEqual(summary["parameters"]["candidates"], 2)
        self.assertEqual(summary["parameters"]["agents"], 1)
        self.assertEqual(summary["dataLibrary"][0], {"source": "s0", "predicate": "omega", "target": "s2"})
        self.assertEqual(summary["probeLibrary"][0], {"from": "s0", "to": "s1"})
        self.assertEqual(summary["steps"][0]["name"], "Step 1")
        self.assertEqual(summary["steps"][0]["items"][1]["target"], "s5")


if __name__ == "__main__":
    unittest.main()
