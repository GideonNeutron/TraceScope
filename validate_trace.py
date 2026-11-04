import json,sys
x=json.load(open(sys.argv[1]))
e=x['traceEvents'];m=x['metadata']
assert len(e)==m['retainedEvents']<=m['capacity']
assert m['overwrittenEvents']==m['totalEvents']-len(e)
assert all(v['ph']=='X' and v['dur']>=0 and isinstance(v['tid'],int) for v in e)
if 'test_' in sys.argv[1]:
 assert e[-2]['name']=='quote" slash\\ newline\n'
 assert len(e[-1]['name'])==47
if 'demo_' in sys.argv[1]:
 assert len(e)==8000 and m['overwrittenEvents']==0
 assert {v['tid'] for v in e}=={0,1,2,3}
 assert sum(v['name']=='synthetic_frame' for v in e)==4000
print('PASS JSON schema, event counts, durations, escaping/threads:',sys.argv[1])
